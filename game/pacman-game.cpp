#include "pacman-game.h"
#include <material.h>
#include <input.h>

#include <glm/gtx/string_cast.hpp>

const char *PacmanGame::name				   = "PacmanGame";
const char *PacmanGame::PacmanEntityData::name = "PacmanGame::PacmanEntity";

template <class T>
T **makeMap(std::size_t width, std::size_t height, const T &def) {
	T **map = new T *[height];
	for (std::size_t i = 0; i < height; ++i) {
		map[i] = new T[width];
		memset(map[i], def, width * sizeof(T));
	}
	return map;
}

template <class T>
void deleteMap(T **map, std::size_t width, std::size_t height) {
	for (std::size_t i = 0; i < height; ++i) {
		delete[] map[i];
	}
	delete[] map;
}

PacmanGame::PacmanGame(ygl::Scene *scene, const std::string &map_file, std::size_t width, std::size_t height)
	: ISystem(scene), width(width), height(height) {
	std::ifstream in(map_file);

	if (!in) { dbLog(ygl::LOG_ERROR, "Cannot open file: ", map_file, " : ", std::strerror(errno)); }

	distanceMap		= makeMap<int>(width, height, -1);
	homeDistanceMap = makeMap<int>(width, height, -1);

	map = new char *[height];
	for (std::size_t i = 0; i < height; ++i) {
		map[i] = new char[width + 1];
		in.getline(map[i], width + 1, '\n');
		std::cout << map[i] << std::endl;
	}

	in.close();
}

void PacmanGame::createMap(ygl::Renderer *renderer, ygl::AssetManager *asman) {
	stbi_uc *buff = new stbi_uc[width * height * 4];
	for (std::size_t y = 0; y < height; ++y) {
		for (std::size_t x = 0; x < width; ++x) {
			buff[(y * width + x) * 4 + 0] = 0;
			buff[(y * width + x) * 4 + 1] = 0;
			buff[(y * width + x) * 4 + 2] = (map[height - y - 1][x] == '#') * 255;
			buff[(y * width + x) * 4 + 3] = 255;
		}
	}

	mapTexture = new ygl::Texture2d(width, height, ygl::ITexture::Type::RGBA, buff);
	mapTexture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	mapTexture->unbind();
	delete[] buff;

	GLuint		  mapTextureIndex = asman->addTexture(mapTexture, "mapTexture");
	ygl::Material mapMat;
	mapMat.use_albedo_map = 1.0;
	mapMat.albedo_map	  = mapTextureIndex;

	ygl::Mesh *quadMesh = new ygl::QuadMesh(1);
	quadMeshIndex		= asman->addMesh(quadMesh, "mapQuad");

	unsigned int mapMatIndex = renderer->addMaterial(mapMat);

	mapQuad = scene->createEntity();
	scene->addComponent<ygl::Transformation>(
		mapQuad, ygl::Transformation(glm::vec3(), glm::vec3(), glm::vec3(width, height, 1)));
	scene->addComponent<ygl::RendererComponent>(mapQuad, ygl::RendererComponent(-1, quadMeshIndex, mapMatIndex));
}

void PacmanGame::createPacman(ygl::Renderer *renderer, ygl::AssetManager *asman) {
	ygl::Texture2d *pacmanTexture = new ygl::Texture2d("./pacman.png", ygl::ITexture::Type::SRGBA);
	pacmanTexture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	pacmanTexture->unbind();

	ygl::Material pacmanMat;
	pacmanMat.use_albedo_map = 1.0;
	pacmanMat.albedo_map	 = asman->addTexture(pacmanTexture, "pacman_texture");

	unsigned int pacmanMatIndex = renderer->addMaterial(pacmanMat);

	lastPlayerPosition			  = glm::ivec2(10, 12);
	glm::vec2 worldPlayerPosition = mapToWorld(lastPlayerPosition);

	pacman = scene->createEntity();
	scene->addComponent<ygl::Transformation>(
		pacman, ygl::Transformation(glm::vec3(worldPlayerPosition.x, worldPlayerPosition.y, 0.5)));
	scene->addComponent<ygl::RendererComponent>(pacman, ygl::RendererComponent(-1, quadMeshIndex, pacmanMatIndex));
	scene->addComponent<PacmanEntityData>(pacman, PacmanEntityData(false, 8.));

	ygl::Keyboard::addKeyCallback([this](GLFWwindow *window, int key, int scancode, int action, int mods) -> void {
		if (window != this->window->getHandle()) return;
		if (action == GLFW_PRESS) {
			PacmanEntityData &data = scene->getComponent<PacmanEntityData>(pacman);
			switch (key) {
				case GLFW_KEY_UP: data.inputDirection = UP; break;
				case GLFW_KEY_DOWN: data.inputDirection = DOWN; break;
				case GLFW_KEY_LEFT: data.inputDirection = LEFT; break;
				case GLFW_KEY_RIGHT: data.inputDirection = RIGHT; break;
			}
		}
		if (action == GLFW_RELEASE) {
			if (key == GLFW_KEY_H) setGhostsState(GO_HOME);
			if (key == GLFW_KEY_G) setGhostsState(CHASE);
			if (key == GLFW_KEY_J) setGhostsState(RUN);
		}
	});
}

void PacmanGame::createGhosts(ygl::Renderer *renderer, ygl::AssetManager *asman) {
	ygl::Texture2d *ghostTextureMask = new ygl::Texture2d("./ghost_mask.png", ygl::ITexture::Type::SRGBA);
	ghostTextureMask->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	ghostTextureMask->unbind();
	ygl::Texture2d *ghostTextureEyes = new ygl::Texture2d("./ghost_eyes.png", ygl::ITexture::Type::SRGBA);
	ghostTextureEyes->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	ghostTextureEyes->unbind();
	unsigned int textureMaskIndex = asman->addTexture(ghostTextureMask, "ghost_mask");
	unsigned int textureEyesIndex = asman->addTexture(ghostTextureEyes, "ghost_eyes");

	ygl::VFShader *ghostShader		= new ygl::VFShader("./shaders/unlit.vs", "./shaders/ghost/ghost.fs");
	unsigned int   ghostShaderIndex = asman->addShader(ghostShader, "ghost_shader");

	ygl::Material deadMat;
	deadMat.albedo		   = glm::vec3(1.f, 1.f, 1.f);
	deadMat.use_albedo_map = 0.0;
	deadMat.use_ao_map	   = 1.0;
	deadMat.albedo_map	   = textureMaskIndex;
	deadMat.ao_map		   = textureEyesIndex;
	deadMatIdx			   = renderer->addMaterial(deadMat);

	ygl::Material weakMat;
	weakMat.albedo		   = glm::vec3(1.f, 1.f, 1.f);
	weakMat.use_albedo_map = 1.0;
	weakMat.use_ao_map	   = 0.0;
	weakMat.albedo_map	   = textureMaskIndex;
	weakMat.ao_map		   = textureEyesIndex;
	weakMatIdx			   = renderer->addMaterial(weakMat);

	const std::size_t colors_count		   = 4;
	glm::vec3		  colors[colors_count] = {
		glm::vec3(236 / 255.f, 0 / 255.f, 5 / 255.f),
		glm::vec3(12 / 255.f, 173 / 255.f, 228 / 255.f),
		glm::vec3(240 / 255.f, 131 / 255.f, 0 / 255.f),
		glm::vec3(246 / 255.f, 156 / 255.f, 182 / 255.f),
	};

	std::size_t count = 0;
	for (std::size_t y = 0; y < height; ++y) {
		for (std::size_t x = 0; x < width; ++x) {
			if (map[y][x] == 'g') {
				ygl::Entity ghost = scene->createEntity();

				ygl::Material mat;
				mat.albedo			= colors[count % colors_count];
				mat.use_albedo_map	= 1.0;
				mat.use_ao_map		= 1.0;
				mat.albedo_map		= textureMaskIndex;
				mat.ao_map			= textureEyesIndex;
				unsigned int matIdx = renderer->addMaterial(mat);

				scene->addComponent(ghost, ygl::Transformation(glm::vec3(mapToWorld(glm::ivec2(x, y)), 0.6f)));
				scene->addComponent(ghost, ygl::RendererComponent(ghostShaderIndex, quadMeshIndex, matIdx));
				PacmanEntityData &data = scene->addComponent(ghost, PacmanEntityData(true, 5. + (rand() % 2)));
				data.originalMatIdx	   = matIdx;

				++count;
			}
		}
	}
}

void PacmanGame::setGhostsState(State state) {
	for (ygl::Entity e : this->entities) {
		PacmanEntityData	   &data = scene->getComponent<PacmanEntityData>(e);
		ygl::RendererComponent &rc	 = scene->getComponent<ygl::RendererComponent>(e);
		if (data.isAI) {
			data.ai_state = state;
			switch (state) {
				case STAY: rc.materialIndex = data.originalMatIdx; break;
				case CHASE: rc.materialIndex = data.originalMatIdx; break;
				case RUN: rc.materialIndex = weakMatIdx; break;
				case GO_HOME: rc.materialIndex = deadMatIdx; break;
			}
		}
	}
}

void PacmanGame::init() {
	// require an asset manager and a renderer
	ygl::AssetManager *asman	= scene->getSystem<ygl::AssetManager>();
	ygl::Renderer	  *renderer = scene->getSystem<ygl::Renderer>();

	scene->registerComponent<PacmanEntityData>();
	scene->setSystemSignature<PacmanGame, ygl::Transformation, PacmanEntityData>();

	window = renderer->getWindow();

	createMap(renderer, asman);
	createPacman(renderer, asman);
	createGhosts(renderer, asman);

	fillDistanceMap(distanceMap, lastPlayerPosition);
	fillDistanceMap(homeDistanceMap, glm::ivec2(10, 10));
}

glm::ivec2 PacmanGame::worldToMap(glm::vec2 position) {
	glm::ivec2 res = glm::round(position - glm::vec2(-(width / 2.f) + 0.5, height / 2.f - 0.5));
	return glm::ivec2(res.x, -res.y);
}

glm::vec2 PacmanGame::mapToWorld(glm::ivec2 position) {
	position.y = -position.y;
	return glm::vec2(position) + glm::vec2(-(width / 2.f) + 0.5, height / 2.f - 0.5);
}

// checks if a position on the map is free
bool PacmanGame::isFree(unsigned int x, unsigned int y) {
	if (y >= height || x >= width) return true;
	return map[y][x] != '#';
}

bool PacmanGame::isFree(glm::ivec2 pos) { return isFree(pos.x, pos.y); }

// gets the unit vector of a direction (in map array coordinates)
glm::ivec2 PacmanGame::getMapVector(Direction dir) {
	switch (dir) {
		case LEFT: return glm::ivec2(-1, 0);
		case RIGHT: return glm::ivec2(1, 0);
		case UP: return glm::ivec2(0, -1);
		case DOWN: return glm::ivec2(0, 1);
	}
	return glm::ivec2(0, 0);
}
glm::vec2 PacmanGame::getWorldVector(Direction dir) {
	switch (dir) {
		case LEFT: return glm::vec2(-1, 0);
		case RIGHT: return glm::vec2(1, 0);
		case UP: return glm::vec2(0, 1);
		case DOWN: return glm::vec2(0, -1);
	}
	return glm::ivec2(0, 0);
}

// checks in a direction from a given position
bool PacmanGame::isFree(glm::ivec2 pos, Direction direction) {
	glm::ivec2 delta = getMapVector(direction);
	return isFree(pos.x + delta.x, pos.y + delta.y);
}

// tries to go in inputDirection if it is perpendicular to moveDirection
bool PacmanGame::tryDirection(Direction inputDirection, Direction &moveDirection, glm::ivec2 posOnMap) {
	if (std::abs(inputDirection - moveDirection) % 2 == 0) return false;	 // cannot go in reverse or forward
	if (isFree(posOnMap, inputDirection)) {
		moveDirection = inputDirection;
		return true;
	}
	return false;
}

void PacmanGame::printDistanceMap(int **distanceMap) {
	for (std::size_t y = 0; y < height; ++y) {
		for (std::size_t x = 0; x < width; ++x) {
			std::cout << std::setw(4);
			if (distanceMap[y][x] == -1) {
				std::cout << " ";
			} else std::cout << distanceMap[y][x];
		}
		std::cout << std::endl;
	}
}

void PacmanGame::updatePacmanEntity(PacmanEntityData &data, ygl::Transformation &transform) {
	Direction &moveDirection  = data.moveDirection;
	Direction &inputDirection = data.inputDirection;

	glm::vec2  objectPos = glm::vec2(transform.position.x, transform.position.y);
	glm::ivec2 posOnMap	 = worldToMap(objectPos);
	glm::vec2  markerPos =
		mapToWorld(posOnMap);	  // the rounded coordinates, they "mark" the position on the map array

	if (moveDirection == NONE) moveDirection = inputDirection;

	if (moveDirection != inputDirection) {
		if (std::abs(moveDirection - inputDirection) == 2) { moveDirection = inputDirection; }
	}

	glm::vec2 objectToMarker = objectPos - markerPos;	  // difference vector from marker to object

	switch (moveDirection) {
		case UP: transform.rotation.z = -M_PI / 2; break;
		case DOWN: transform.rotation.z = M_PI / 2; break;
		case LEFT: transform.rotation.z = 0; break;
		case RIGHT: transform.rotation.z = M_PI; break;
	}

	// move in a direction
	glm::vec2 worldMovement = getWorldVector(moveDirection);
	transform.position.x += worldMovement.x * data.speed * window->deltaTime;
	transform.position.y += worldMovement.y * data.speed * window->deltaTime;

	// if we have just passed the center of a square, then we must perform a collision check
	if (glm::dot(objectToMarker, worldMovement) > 0.01) {
		if (tryDirection(inputDirection, moveDirection,
						 posOnMap)) {	  // see if the player wants to turn and if it is possible
			transform.position.y = markerPos.y;
			transform.position.x = markerPos.x;
		} else if (!isFree(posOnMap, moveDirection)) {	   // else, see if there is a wall in front of the player
			transform.position.x = markerPos.x;
			transform.position.y = markerPos.y;		// if yes, stop
			moveDirection		 = NONE;
			inputDirection		 = NONE;
		}
	}

	if (transform.position.x >= width / 2.f - 0.1) { transform.position.x = -(width / 2.f) + 0.2; }
	if (transform.position.x <= -(width / 2.f) + 0.1) { transform.position.x = width / 2.f - 0.2; }

	transform.updateWorldMatrix();
}

template <class T>
T &get(T **arr, glm::ivec2 pos) {
	return arr[pos.y][pos.x];
}

// simple BFS
void PacmanGame::fillDistanceMap(int **distanceMap, glm::ivec2 start) {
	for (std::size_t i = 0; i < height; ++i) {
		memset(distanceMap[i], -1, width * sizeof(int));
	}

	std::queue<glm::ivec2> q;
	get(distanceMap, start) = 0;
	q.push(start);

	auto bfs_step = [this, distanceMap](PacmanGame::Direction dir, glm::ivec2 pos, std::queue<glm::ivec2> &q,
										int dist) {
		glm::ivec2 neighbour = pos + getMapVector(dir);
		if (isFree(neighbour) && get(distanceMap, neighbour) == -1) {
			get(distanceMap, neighbour) = dist + 1;
			q.push(neighbour);
		}
	};

	while (!q.empty()) {
		glm::ivec2 pos	= q.front();
		int		   dist = get(distanceMap, pos);
		q.pop();

		bfs_step(UP, pos, q, dist);
		bfs_step(DOWN, pos, q, dist);
		bfs_step(LEFT, pos, q, dist);
		bfs_step(RIGHT, pos, q, dist);
	}
}

void PacmanGame::doWork() {
	auto go_to_player = [this](Direction dir, glm::ivec2 position, PacmanEntityData &data, int dist) {
		glm::ivec2 neigh = position + getMapVector(dir);
		if (distanceMap[neigh.y][neigh.x] == dist - 1) { data.inputDirection = (dir); }
	};

	auto run_from_player = [this](Direction dir, glm::ivec2 position, PacmanEntityData &data, int dist) {
		glm::ivec2 neigh = position + getMapVector(dir);
		if (distanceMap[neigh.y][neigh.x] == dist + 1) { data.inputDirection = (dir); }
	};

	auto go_home = [this](Direction dir, glm::ivec2 position, PacmanEntityData &data, int dist) {
		glm::ivec2 neigh = position + getMapVector(dir);
		if (homeDistanceMap[neigh.y][neigh.x] == dist - 1) { data.inputDirection = (dir); }
	};

	for (ygl::Entity e : this->entities) {
		ygl::Transformation &transform = scene->getComponent<ygl::Transformation>(e);
		PacmanEntityData	&data	   = scene->getComponent<PacmanEntityData>(e);

		if (data.isAI) {
			glm::ivec2	position = worldToMap(transform.position);
			int			dist;
			std::size_t start = e % 4;

			switch (data.ai_state) {
				case State::CHASE:
					dist = get(distanceMap, position);
					for (std::size_t i = 0; i < 4; ++i) {
						Direction dir = Direction((start + i) % 4);
						go_to_player(dir, position, data, dist);
					}
					break;
				case State::RUN:
					dist = get(distanceMap, position);
					for (std::size_t i = 0; i < 4; ++i) {
						Direction dir = Direction((start + i) % 4);
						run_from_player(dir, position, data, dist);
					}
					break;
				case State::GO_HOME:
					dist = get(homeDistanceMap, position);
					for (std::size_t i = 0; i < 4; ++i) {
						Direction dir = Direction((start + i) % 4);
						go_home(dir, position, data, dist);
					}
					break;
			}
		} else {
			glm::ivec2 playerPosition = worldToMap(transform.position);
			if (playerPosition != lastPlayerPosition) {
				fillDistanceMap(distanceMap, playerPosition);
				lastPlayerPosition = playerPosition;
			}
		}

		updatePacmanEntity(data, transform);
	}
}

PacmanGame::~PacmanGame() {
	deleteMap(map, width, height);
	deleteMap(distanceMap, width, height);
	deleteMap(homeDistanceMap, width, height);
}

void PacmanGame::write(std::ostream &out){THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED")};
void PacmanGame::read(std::istream &in){THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED")};