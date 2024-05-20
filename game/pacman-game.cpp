#include "pacman-game.h"
#include <yoghurtgl.h>
#include <material.h>
#include <input.h>

#include <glm/gtx/string_cast.hpp>

const char *PacmanGame::name				   = "PacmanGame";
const char *PacmanGame::PacmanEntityData::name = "PacmanGame::PacmanEntity";

// utility functions to create 2d dynamic arrays
// it will be faster to use 1d arrays and index them differently, but for a small map it does not matter

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

template <class T>
T &get(T **arr, glm::ivec2 pos) {
	return arr[pos.y][pos.x];
}

PacmanGame::PacmanGame(ygl::Scene *scene, const std::string &map_file, std::size_t width, std::size_t height)
	: ISystem(scene), width(width), height(height) {
	std::ifstream in(map_file);

	if (!in) { dbLog(ygl::LOG_ERROR, "Cannot open file: ", map_file, " : ", std::strerror(errno)); }

	map = new char *[height];
	for (std::size_t i = 0; i < height; ++i) {
		map[i] = new char[width + 1];
		in.getline(map[i], width + 1, '\n');
		if (strlen(map[i]) < width) THROW_RUNTIME_ERR("Incorrect input dimensions or corrupted map file");
	}
	if (!in.eof()) dbLog(ygl::LOG_WARNING, "did not read the entire map file!!");
	in.close();

	// initialize distance fields
	distanceMap		= makeMap<int>(width, height, -1);
	homeDistanceMap = makeMap<int>(width, height, -1);

	score	= 0;
	lives	= gameSettings.pacmanLives;

	// load GUI font
	ImGuiIO io = ImGui::GetIO();
	font	   = io.Fonts->AddFontFromFileTTF("./resources/ProggyClean.ttf", 30);
}

// creates the map entity
void PacmanGame::createMap(ygl::Renderer *renderer, ygl::AssetManager *asman) {
	// create texture data for the map renderer
	stbi_uc *buff = new stbi_uc[width * height * 4];
	unsigned int allDots = 0;
	for (std::size_t y = 0; y < height; ++y) {
		for (std::size_t x = 0; x < width; ++x) {
			if (map[height - y - 1][x] == '.' || map[height - y - 1][x] == '@') { ++allDots; }
			if (map[height - y - 1][x] == 'h') homePosition = glm::ivec2(x, height - y - 1);
			if (map[height - y - 1][x] == 'p') pacmanStartPosition = glm::ivec2(x, height - y - 1);

			buff[(y * width + x) * 4 + 0] = (map[height - y - 1][x] == '@') * 255;
			buff[(y * width + x) * 4 + 1] = (map[height - y - 1][x] == '.') * 255;
			buff[(y * width + x) * 4 + 2] = (map[height - y - 1][x] == '#') * 255;
			buff[(y * width + x) * 4 + 3] = 255;
			// image Y is flipped because the UV-s of the quad are flipped
		}
	}
	currentDots = allDots;

	// create map texture
	mapTexture = new ygl::Texture2d(width, height, ygl::TextureType::RGBA16F, buff);
	mapTexture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	mapTexture->unbind();
	delete[] buff;
	GLuint mapTextureIndex = asman->addTexture(mapTexture, "mapTexture");

	// map material
	ygl::Material mapMat;
	mapMat.use_albedo_map	 = 1.0;
	mapMat.albedo_map		 = mapTextureIndex;
	unsigned int mapMatIndex = renderer->addMaterial(mapMat);

	// map shader
	ygl::VFShader *mapShader = new ygl::VFShader("./shaders/unlit.vs", "./shaders/pacman/map.fs");
	mapShader->bind();
	mapShader->setUniform("resolution", glm::ivec2(width, height));
	mapShader->unbind();
	unsigned int mapShaderIndex = asman->addShader(mapShader, "map_shader");

	// quad mesh, used to render all sprites
	ygl::Mesh *quadMesh = new ygl::QuadMesh(1);
	quadMesh->setDepthFunc(GL_LEQUAL);
	quadMeshIndex = asman->addMesh(quadMesh, "mapQuad");

	// create the map entity
	mapQuad = scene->createEntity();
	scene->addComponent<ygl::Transformation>(
		mapQuad, ygl::Transformation(glm::vec3(), glm::vec3(), glm::vec3(width, height, 1)));
	scene->addComponent<ygl::RendererComponent>(mapQuad,
												ygl::RendererComponent(mapShaderIndex, quadMeshIndex, mapMatIndex));
}

// creates the player entity (Pacman)
void PacmanGame::createPacman(ygl::Renderer *renderer, ygl::AssetManager *asman) {
	// texture
	ygl::Texture2d *pacmanTexture = new ygl::Texture2d("./resources/pacman.png", ygl::TextureType::SRGBA8);
	pacmanTexture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	pacmanTexture->unbind();

	// material
	ygl::Material pacmanMat;
	pacmanMat.use_albedo_map	= 1.0;
	pacmanMat.albedo_map		= asman->addTexture(pacmanTexture, "pacman_texture");
	unsigned int pacmanMatIndex = renderer->addMaterial(pacmanMat);

	// position
	lastPlayerPosition			  = pacmanStartPosition;
	glm::vec2 worldPlayerPosition = mapToWorld(lastPlayerPosition);

	// creating the player entity
	pacman = scene->createEntity();
	scene->addComponent<ygl::Transformation>(
		pacman, ygl::Transformation(glm::vec3(worldPlayerPosition.x, worldPlayerPosition.y, 0.5)));

	scene->addComponent<ygl::RendererComponent>(pacman, ygl::RendererComponent(-1, quadMeshIndex, pacmanMatIndex));

	PacmanEntityData &pacmanData =
		scene->addComponent<PacmanEntityData>(pacman, PacmanEntityData(false, gameSettings.pacmanSpeed));
	pacmanData.startPosition = pacmanStartPosition;

	// add a key callback that controlls the character and starts the game
	ygl::Keyboard::addKeyCallback([this](GLFWwindow *window, int key, int scancode, int action, int mods) -> void {
		if (window != this->window->getHandle()) return;
		if (!(this->gameStarted)) {
			setGhostsState(CHASE);
			this->gameStarted = true;
		}
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

// generates random speed for a ghost
float PacmanGame::generateGhostSpeed() {
	return gameSettings.ghostBaseSpeed + (rand() % 10) / 10.f * gameSettings.ghostSpeedRandomCoef;
}

// creates all the ghosts, marked on the map
void PacmanGame::createGhosts(ygl::Renderer *renderer, ygl::AssetManager *asman) {
	// textures
	ygl::Texture2d *ghostTextureMask = new ygl::Texture2d("./resources/ghost_mask.png", ygl::TextureType::SRGBA8);
	ghostTextureMask->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	ghostTextureMask->unbind();
	ygl::Texture2d *ghostTextureEyes = new ygl::Texture2d("./resources/ghost_eyes.png", ygl::TextureType::SRGBA8);
	ghostTextureEyes->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	ghostTextureEyes->unbind();
	unsigned int textureMaskIndex = asman->addTexture(ghostTextureMask, "ghost_mask");
	unsigned int textureEyesIndex = asman->addTexture(ghostTextureEyes, "ghost_eyes");

	// shader
	ygl::VFShader *ghostShader		= new ygl::VFShader("./shaders/unlit.vs", "./shaders/pacman/ghost.fs");
	unsigned int   ghostShaderIndex = asman->addShader(ghostShader, "ghost_shader");

	// materials
	ygl::Material deadMat;	   // only eyes
	deadMat.albedo		   = glm::vec3(1.f, 1.f, 1.f);
	deadMat.use_albedo_map = 0.0;
	deadMat.use_ao_map	   = 1.0;
	deadMat.albedo_map	   = textureMaskIndex;
	deadMat.ao_map		   = textureEyesIndex;
	deadMatIdx			   = renderer->addMaterial(deadMat);

	ygl::Material weakMat;	   // purple scared ghost
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

	// iterate over the map, find the 'g' symbols and create ghosts there
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
				PacmanEntityData &data = scene->addComponent(ghost, PacmanEntityData(true, generateGhostSpeed()));
				data.originalMatIdx	   = matIdx;
				data.startPosition	   = glm::ivec2(x, y);

				++count;
			}
		}
	}
}

// sets the state of the ghost. Synchronizes state, speed and material data
void PacmanGame::setGhostState(ygl::Entity e, PacmanEntityData &data, std::function<State(State)> f) {
	if (data.isAI) {
		ygl::RendererComponent &rc		 = scene->getComponent<ygl::RendererComponent>(e);
		State					newState = f(data.aiState);
		if (newState == data.aiState) return;
		data.aiState = newState;
		switch (data.aiState) {
			case STAY:
				rc.materialIndex = data.originalMatIdx;
				data.speed		 = generateGhostSpeed();
				break;
			case CHASE:
				rc.materialIndex = data.originalMatIdx;
				data.speed		 = generateGhostSpeed();
				break;
			case RUN:
				rc.materialIndex = weakMatIdx;
				data.speed		 = gameSettings.weakGhostSpeed;
				break;
			case GO_HOME:
				rc.materialIndex = deadMatIdx;
				data.speed		 = gameSettings.deadGhostSpeed;
				break;
		}
	}
}

// sets the state of all ghosts according to the function f
void PacmanGame::setGhostsState(std::function<State(State)> f) {
	for (ygl::Entity e : this->entities) {
		PacmanEntityData &data = scene->getComponent<PacmanEntityData>(e);
		setGhostState(e, data, f);
	}
}

// set a single state for all ghosts
void PacmanGame::setGhostsState(State state) {
	setGhostsState([state](State) { return state; });
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
	fillDistanceMap(homeDistanceMap, homePosition);
}

// coordinate system conversions
glm::ivec2 PacmanGame::worldToMap(glm::vec2 position) {
	glm::ivec2 res = glm::round(position - glm::vec2(-(width / 2.f) + 0.5, height / 2.f - 0.5));
	return glm::ivec2(res.x, -res.y);
}

glm::vec2 PacmanGame::mapToWorld(glm::ivec2 position) {
	position.y = -position.y;
	return glm::vec2(position) + glm::vec2(-(width / 2.f) + 0.5, height / 2.f - 0.5);
}

// checks if a position on the map is free
bool PacmanGame::isFree(unsigned int x, unsigned int y) { return isFree(x, y, false); }

// checks if a position on the map is free
bool PacmanGame::isFree(unsigned int x, unsigned int y, bool onFail) {
	if (y >= height || x >= width) return onFail;
	return map[y][x] != '#';
}

// checks if a position on the map is free
bool PacmanGame::isFree(glm::ivec2 pos) { return isFree(pos.x, pos.y); }

// gets the unit vector of a direction (in map array coordinates)
glm::ivec2 PacmanGame::getMapVector(Direction dir) {
	switch (dir) {
		case LEFT: return glm::ivec2(-1, 0);
		case RIGHT: return glm::ivec2(1, 0);
		case UP: return glm::ivec2(0, -1);
		case DOWN: return glm::ivec2(0, 1);
		case NONE: return glm::ivec2(0, 0);
	}
}
// gets the unit vector of a direction (in world coordinates)
glm::vec2 PacmanGame::getWorldVector(Direction dir) {
	switch (dir) {
		case LEFT: return glm::vec2(-1, 0);
		case RIGHT: return glm::vec2(1, 0);
		case UP: return glm::vec2(0, 1);
		case DOWN: return glm::vec2(0, -1);
		case NONE: return glm::ivec2(0, 0);
	}
}

// checks in a direction from a given position
bool PacmanGame::isFree(glm::ivec2 pos, Direction direction, bool onFail) {
	glm::ivec2 delta = getMapVector(direction);
	return isFree(pos.x + delta.x, pos.y + delta.y, onFail);
}

// checks in a direction from a given position
bool PacmanGame::isFree(glm::ivec2 pos, Direction direction) { return isFree(pos, direction, false); }

// tries to go in inputDirection if it is perpendicular to moveDirection
bool PacmanGame::tryDirection(Direction inputDirection, Direction &moveDirection, glm::ivec2 posOnMap) {
	if (std::abs(inputDirection - moveDirection) % 2 == 0) return false;	 // cannot go in reverse or forward
	if (isFree(posOnMap, inputDirection)) {
		moveDirection = inputDirection;
		return true;
	}
	return false;
}

// print the distance map for debugging purposes
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

// updates any entity, managed by this system, a ghost or player
void PacmanGame::updatePacmanEntity(PacmanEntityData &data, ygl::Transformation &transform) {
	// renaming for convenience
	Direction &moveDirection  = data.moveDirection;
	Direction &inputDirection = data.inputDirection;

	// compute coordinates in all coordinate systems
	glm::vec2  objectPos = glm::vec2(transform.position.x, transform.position.y);	  // world position of object
	glm::ivec2 posOnMap	 = worldToMap(objectPos);									  // map position of object
	glm::vec2  markerPos =
		mapToWorld(posOnMap);	  // the rounded world coordinates, they "mark" the position on the map array

	// if it is not moving, input is translated to action
	if (moveDirection == NONE) moveDirection = inputDirection;

	// going in the opposite direction is allowed at all times
	if (moveDirection != inputDirection) {
		if (std::abs(moveDirection - inputDirection) == 2) { moveDirection = inputDirection; }
	}

	glm::vec2 objectToMarker = objectPos - markerPos;	  // difference vector from marker to object

	// make the object rotate based on movement direction
	switch (moveDirection) {
		case UP: transform.rotation.z = -M_PI / 2; break;
		case DOWN: transform.rotation.z = M_PI / 2; break;
		case LEFT: transform.rotation.z = 0; break;
		case RIGHT: transform.rotation.z = M_PI; break;
		case NONE: transform.rotation.z = 0; break;
	}

	// move in a direction
	glm::vec2 worldMovement = getWorldVector(moveDirection);
	transform.position.x += worldMovement.x * data.speed * window->deltaTime;
	transform.position.y += worldMovement.y * data.speed * window->deltaTime;

	// main collision detection:
	// if we have just passed the center of a square, then we must perform a collision check
	if (glm::dot(objectToMarker, worldMovement) > 0.01) {
		if (tryDirection(inputDirection, moveDirection,
						 posOnMap)) {	  // see if the player wants to turn and if it is possible
			transform.position.y = markerPos.y;
			transform.position.x = markerPos.x;
		} else if (!isFree(posOnMap, moveDirection, true)) {	 // else, see if there is a wall in front of the player
			// note that here the outside of the map is considered empty so that portals can be used
			transform.position.x = markerPos.x;
			transform.position.y = markerPos.y;		// if yes, stop
			moveDirection		 = NONE;
			inputDirection		 = NONE;
		}
	}

	// teleportation
	if (transform.position.x >= width / 2.f - 0.1) { transform.position.x = -(width / 2.f) + 0.2; }
	if (transform.position.x <= -(width / 2.f) + 0.1) { transform.position.x = width / 2.f - 0.2; }

	transform.updateWorldMatrix();
}

// simple BFS that creates a "flow field" for the ghosts to move on
void PacmanGame::fillDistanceMap(int **distanceMap, glm::ivec2 start) {
	// clear the distance field
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

// decision makers for ghosts
// later, the these decision functions are called for the four directions in a random order
void go_to_target(int **distanceMap, PacmanGame::Direction dir, glm::ivec2 position, PacmanGame::PacmanEntityData &data,
				  int dist) {
	glm::ivec2 neigh = position + PacmanGame::getMapVector(dir);
	if (distanceMap[neigh.y][neigh.x] == dist - 1) { data.inputDirection = (dir); }
};

void run_from_target(int **distanceMap, PacmanGame::Direction dir, glm::ivec2 position,
					 PacmanGame::PacmanEntityData &data, int dist) {
	glm::ivec2 neigh = position + PacmanGame::getMapVector(dir);
	if (distanceMap[neigh.y][neigh.x] == dist + 1) { data.inputDirection = (dir); }
};

// looks in the four directions and decides where the ghost should go by setting its inputDirection
// effectively mimicking user input
void PacmanGame::resolveAIState(
	int **distanceMap, glm::ivec2 position, unsigned char start, PacmanEntityData &data,
	std::function<void(int **, Direction, glm::ivec2, PacmanEntityData &, int)> tryDirection) {
	int dist = get(distanceMap, position);
	for (std::size_t i = 0; i < 4; ++i) {
		Direction dir = Direction((start + i) % 4);
		tryDirection(distanceMap, dir, position, data, dist);
	}
}

// the entire ghost AI. (figuratively A four-state finite automata)
void PacmanGame::ghostAI(ygl::Entity e, ygl::Transformation &transform, PacmanEntityData &data) {
	glm::ivec2	  position = worldToMap(transform.position);
	unsigned char start = e % 4;

	switch (data.aiState) {
		case State::CHASE: resolveAIState(distanceMap, position, start, data, ::go_to_target); break;
		case State::RUN: resolveAIState(distanceMap, position, start, data, ::run_from_target); break;
		case State::GO_HOME: resolveAIState(homeDistanceMap, position, start, data, ::go_to_target); break;
		case State::STAY: break;
	}
}

// pacman eats a dot on a position
void PacmanGame::eatDot(glm::ivec2 position) {
	// delete dot on map
	get(map, position) = ' ';

	// delete dot on texture
	mapTexture->bind(GL_TEXTURE1);
	uchar data[] = {0, 0, 0, 255};
	glActiveTexture(GL_TEXTURE1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, position.x, height - position.y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glActiveTexture(GL_TEXTURE0);
	mapTexture->unbind(GL_TEXTURE1);

	// detect win condition
	--currentDots;
	if (currentDots == 0) {
		gameEnded		= true;
		gameFinishedWin = true;
	}
}

// gameplay logic for the player, without the movement.
void PacmanGame::updatePlayer(ygl::Transformation &transform, PacmanEntityData &data) {
	// update the player
	glm::ivec2 playerPosition = worldToMap(transform.position);
	if (playerPosition != lastPlayerPosition) {
		// recalculate ghosts pathfinding
		fillDistanceMap(distanceMap, playerPosition);
		lastPlayerPosition = playerPosition;

		// erase dot
		char current = get(map, playerPosition);
		if (current == '.') {	  // eating a dot
			score += gameSettings.eatDotScore;
			eatDot(playerPosition);
		} else if (current == '@') {	 // eating a pill
			score += gameSettings.eatPillScore;
			eatDot(playerPosition);
			pillTimer = Timer();
			// make the ghosts run
			setGhostsState([](State state) -> State {
				switch (state) {
					case GO_HOME: return GO_HOME;
					default: return RUN;
				}
			});
		}
	}
}

// check for collision between a ghost, a player or the ghosts respawn point
void PacmanGame::checkCollision(ygl::Entity ghost, ygl::Transformation &ghostTransform, PacmanEntityData &ghostData,
								ygl::Transformation &pacmanTransform, PacmanEntityData &pacmanData) {
	// calculate distance
	glm::vec2 pacPos   = glm::vec2(pacmanTransform.position.x, pacmanTransform.position.y);
	glm::vec2 ghostPos = glm::vec2(ghostTransform.position.x, ghostTransform.position.y);
	float	  distance = glm::distance(pacPos, ghostPos);

	// if the player collides with the ghost
	if (distance < 0.7f) {
		if (ghostData.aiState == CHASE && !gameSettings.godMode) {
			--lives;
			restartAfterDeath();
			if (lives == 0) { gameEnded = true; }
		}
		if (ghostData.aiState == RUN) {
			setGhostState(ghost, ghostData, [](State) { return GO_HOME; });
		}
	}

	// if the ghost has reached the spawn
	if (glm::distance(ghostPos, mapToWorld(homePosition)) < 0.2f) {
		setGhostState(ghost, ghostData, [](State state) {
			switch (state) {
				case GO_HOME: return CHASE;
				default: return state;
			}
		});
	}
}

// checks if the pill timer has run out and removes its effects if so
void PacmanGame::checkPillTimer() {
	if (Timer::toMs(pillTimer.elapsedNs()) > gameSettings.pillEffectDuration) {
		setGhostsState([](State state) -> State {
			switch (state) {
				case RUN: return CHASE;
				default: return state;
			}
		});
	}
}

void PacmanGame::doWork() {
	if (gameEnded) return;

	ygl::Transformation &pacmanTransform = scene->getComponent<ygl::Transformation>(pacman);
	PacmanEntityData	&pacmanData		 = scene->getComponent<PacmanEntityData>(pacman);

	// update player and pill
	checkPillTimer();
	updatePlayer(pacmanTransform, pacmanData);
	updatePacmanEntity(pacmanData, pacmanTransform);

	// iterate through ghosts
	for (ygl::Entity e : this->entities) {
		if (e == pacman) continue;
		PacmanEntityData	&data	   = scene->getComponent<PacmanEntityData>(e);
		ygl::Transformation &transform = scene->getComponent<ygl::Transformation>(e);

		if (data.isAI) {	 // redundant check, but leave it here for future extendability
			ghostAI(e, transform, data);

			checkCollision(e, transform, data, pacmanTransform, pacmanData);
		}

		updatePacmanEntity(data, transform);
	}
}

// resets the game when the player dies
void PacmanGame::restartAfterDeath() {
	ygl::Transformation &pacmanTransform = scene->getComponent<ygl::Transformation>(pacman);
	PacmanEntityData	&pacmanData		 = scene->getComponent<PacmanEntityData>(pacman);
	glm::vec2			 playerPos		 = mapToWorld(pacmanStartPosition);
	pacmanTransform.position.x			 = playerPos.x;
	pacmanTransform.position.y			 = playerPos.y;
	pacmanTransform.rotation.z			 = 0;
	pacmanData.moveDirection			 = NONE;
	pacmanData.inputDirection			 = NONE;

	pacmanTransform.updateWorldMatrix();

	for (ygl::Entity e : this->entities) {
		ygl::Transformation &transform = scene->getComponent<ygl::Transformation>(e);
		PacmanEntityData	&data	   = scene->getComponent<PacmanEntityData>(e);
		if (data.isAI) {
			glm::vec2 startPos	 = mapToWorld(data.startPosition);
			transform.position.x = startPos.x;
			transform.position.y = startPos.y;
			transform.rotation.z = 0;
			transform.updateWorldMatrix();

			data.aiState		= State::STAY;
			data.inputDirection = NONE;
			data.moveDirection	= NONE;
		}
	}

	gameStarted = false;
}

PacmanGame::~PacmanGame() {
	deleteMap(map, width, height);
	deleteMap(distanceMap, width, height);
	deleteMap(homeDistanceMap, width, height);
}

void PacmanGame::write(std::ostream &out){THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED")};
void PacmanGame::read(std::istream &in){THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED")};

// copy-pasta from https://stackoverflow.com/questions/64653747/how-to-center-align-text-horizontally
// has some bugs, but I am willing to allow uncentered text. It is a feature :D
void TextCentered(std::string text) {
	float win_width	 = ImGui::GetWindowSize().x;
	float text_width = ImGui::CalcTextSize(text.c_str()).x;

	// calculate the indentation that centers the text on one line, relative
	// to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
	float text_indentation = (win_width - text_width) * 0.5f;

	// if text is too long to be drawn on one line, `text_indentation` can
	// become too small or even negative, so we check a minimum indentation
	float min_indentation = 20.0f;
	if (text_indentation <= min_indentation) { text_indentation = min_indentation; }

	ImGui::SameLine(text_indentation);
	ImGui::PushTextWrapPos(win_width - text_indentation);
	ImGui::TextWrapped("%s", text.c_str());
	ImGui::PopTextWrapPos();
}


// the gui of the game. Yes, it's hardly readable
void PacmanGame::drawGUI() {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
							 ImGuiWindowFlags_NoInputs;
	ImGui::PushFont(font);

	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(window->getWidth() / 3.f, 50.f));
	ImGui::Begin("score", nullptr, flags);
	ImGui::Text("Score: %d", getScore());
	ImGui::End();

	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::SetNextWindowPos(ImVec2(window->getWidth() * 2 / 3.f, 0.f));
	ImGui::SetNextWindowSize(ImVec2(window->getWidth() / 3.f, 50.f));
	ImGui::Begin("lives", nullptr, flags);
	ImGui::Text("Lives: %d", getLives());
	ImGui::End();

	if (hasGameEnded()) {
		if (isGameWon()) {
			ImGui::SetNextWindowPos(ImVec2(window->getWidth() / 3., window->getHeight() / 2. - 25));
			ImGui::SetNextWindowSize(ImVec2(window->getWidth() / 3., 50));
			ImGui::Begin("You Won", nullptr, flags);
			TextCentered("You Won!");
			ImGui::End();
		} else {
			ImGui::SetNextWindowPos(ImVec2(window->getWidth() / 3., window->getHeight() / 2. - 25));
			ImGui::SetNextWindowSize(ImVec2(window->getWidth() / 3., 50));
			ImGui::Begin("gameOver", nullptr, flags);
			TextCentered("GameOver!");
			ImGui::End();
		}
	}
	if (!hasGameStarted()) {
		ImGui::SetNextWindowPos(ImVec2(window->getWidth() / 4., window->getHeight() / 3.));
		ImGui::SetNextWindowSize(ImVec2(window->getWidth() / 2., window->getHeight() / 3.));
		ImGui::Begin("BeforeStart", nullptr, flags);
		TextCentered("Press any button to start.\n\nControl pacman with arrow keys\n[ESC] to exit");
		ImGui::End();
	}

	ImGui::PopFont();
}
