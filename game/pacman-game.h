#pragma once
#include <yoghurtgl.h>
#include <ecs.h>
#include <mesh.h>
#include <entities.h>
#include <texture.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <fstream>

class PacmanGame : public ygl::ISystem {
   public:
	enum Direction { UP = 0, LEFT = 1, DOWN = 2, RIGHT = 3 , NONE = 4};

	enum State {
		STAY, CHASE, RUN, GO_HOME
	};
	class PacmanEntityData : public ygl::Serializable {
	   public:
		static const char *name;
		Direction		   inputDirection;
		Direction		   moveDirection;
		bool			   isAI;
		float			   speed;
		State			   ai_state;
		unsigned int	   originalMatIdx;

		PacmanEntityData(bool isAI, float speed)
			: inputDirection(NONE), moveDirection(NONE), isAI(isAI), speed(speed), ai_state(STAY) {}
		PacmanEntityData() : PacmanEntityData(true, 7.0) {}

		void serialize(std::ostream &out) { THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED") }
		void deserialize(std::istream &in) { THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED") }
	};

   private:
	std::size_t width, height;

	char		  **map;
	int			   **distanceMap;
	int			   **homeDistanceMap;
	ygl::Texture2d *mapTexture;
	unsigned int	quadMeshIndex;
	unsigned int	 deadMatIdx;
	unsigned int	 weakMatIdx;
	ygl::Entity		mapQuad = -1;
	ygl::Entity		pacman	= -1;

	glm::ivec2 lastPlayerPosition;

	ygl::Window *window;

	float pacmanSpeed = 8;

	void createMap(ygl::Renderer *renderer, ygl::AssetManager *asman);
	void createPacman(ygl::Renderer *renderer, ygl::AssetManager *asman);
	void createGhosts(ygl::Renderer *renderer, ygl::AssetManager *asman);
	void setGhostsState(State state);

	glm::ivec2 worldToMap(glm::vec2 position);
	glm::vec2  mapToWorld(glm::ivec2 position);
	bool	   isFree(unsigned int x, unsigned int y);
	bool	   isFree(glm::ivec2 pos);
	glm::ivec2 getMapVector(Direction dir);
	glm::vec2  getWorldVector(Direction dir);
	bool	   isFree(glm::ivec2 pos, Direction direction);
	bool	   tryDirection(Direction inputDirection, Direction &moveDirection, glm::ivec2 posOnMap);
	void	   printDistanceMap(int **distanceMap);

	void updatePacmanEntity(PacmanEntityData &data, ygl::Transformation &transform);

	void bfs_step(PacmanGame::Direction dir, glm::ivec2 pos, std::queue<glm::ivec2> &q, int dist);
	void fillDistanceMap(int **distanceMap, glm::ivec2 start);

   public:
	static const char *name;

	PacmanGame(ygl::Scene *scene, const std::string &map_file, std::size_t width, std::size_t height);

	void init() override;

	void doWork() override;

	~PacmanGame() override;

	void write(std::ostream &out) override;
	void read(std::istream &in) override;
};
