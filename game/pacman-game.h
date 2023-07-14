#pragma once
#include <yoghurtgl.h>
#include <ecs.h>
#include <mesh.h>
#include <entities.h>
#include <texture.h>
#include <timer.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <fstream>

// global default game settings.
struct PacmanGameSettings {
	float		 pacmanSpeed		  = 8.0f;
	float		 ghostBaseSpeed		  = 5.0f;
	float		 ghostSpeedRandomCoef = 2.0f;
	float		 weakGhostSpeed		  = 3.0f;
	float		 deadGhostSpeed		  = 4.0f;
	uint64_t	 pillEffectDuration	  = 5000;	  // time in ms
	unsigned int eatDotScore		  = 10;
	unsigned int eatPillScore		  = 50;
	unsigned int eatGhostScore		  = 100;
	unsigned int pacmanLives		  = 3;
	bool		 godMode			  = false;
};

class PacmanGame : public ygl::ISystem {
   public:
	enum Direction { UP = 0, LEFT = 1, DOWN = 2, RIGHT = 3, NONE = 4 };

	enum State { STAY, CHASE, RUN, GO_HOME };
	class PacmanEntityData : public ygl::Serializable {
	   public:
		static const char *name;
		Direction		   inputDirection;
		Direction		   moveDirection;
		bool			   isAI;
		float			   speed;
		State			   aiState;
		unsigned int	   originalMatIdx;
		glm::ivec2		   startPosition;

		PacmanEntityData(bool isAI, float speed)
			: inputDirection(NONE), moveDirection(NONE), isAI(isAI), speed(speed), aiState(STAY), originalMatIdx(-1), startPosition(0) {}
		PacmanEntityData() : PacmanEntityData(true, 7.0) {}		// obligatory default constructor because of engine

		void serialize(std::ostream &out) { THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED") }
		void deserialize(std::istream &in) { THROW_RUNTIME_ERR("SERIALIZING A PACMAN GAME IS NOT SUPPORTED") }
	};

	PacmanGameSettings gameSettings; // customizable game settings

   private:
	std::size_t width, height; // dimensions
	
	// map, read from file
	char		  **map;
	// distance fields for path finding
	int			  **distanceMap;
	int			  **homeDistanceMap;
	ygl::Texture2d *mapTexture;
	// indexes for reference in Renderer and AssetManager
	unsigned int	quadMeshIndex;
	unsigned int	deadMatIdx;
	unsigned int	weakMatIdx;

	// unique entities that must be remembered
	ygl::Entity		mapQuad = -1;
	ygl::Entity		pacman	= -1;

	glm::ivec2	 lastPlayerPosition; // used for controlled computation of path finding

	// these are read from the map
	glm::ivec2	 pacmanStartPosition;
	glm::ivec2	 homePosition;

	// window pointer
	ygl::Window *window;

	// global game state
	unsigned int score;
	unsigned int currentDots;
	bool		 gameStarted	 = false;
	bool		 gameEnded		 = false;
	bool		 gameFinishedWin = false;
	unsigned int lives;

	// font for the GUI
	ImFont		*font;

	// counts time from consuming a pill
	Timer pillTimer;

	void createMap(ygl::Renderer *renderer, ygl::AssetManager *asman);
	void createPacman(ygl::Renderer *renderer, ygl::AssetManager *asman);
	void createGhosts(ygl::Renderer *renderer, ygl::AssetManager *asman);
	void setGhostState(ygl::Entity e, PacmanEntityData &data, std::function<State(State)> f);
	void setGhostsState(std::function<State(State)> f);
	void setGhostsState(State state);

	float generateGhostSpeed();

	glm::ivec2 worldToMap(glm::vec2 position);
	glm::vec2  mapToWorld(glm::ivec2 position);
	bool	   isFree(unsigned int x, unsigned int y, bool onFail);
	bool	   isFree(unsigned int x, unsigned int y);
	bool	   isFree(glm::ivec2 pos);
	bool	   isFree(glm::ivec2 pos, Direction direction, bool onFail);
	bool	   isFree(glm::ivec2 pos, Direction direction);


	bool tryDirection(Direction inputDirection, Direction &moveDirection, glm::ivec2 posOnMap);
	void printDistanceMap(int **distanceMap);

	void updatePacmanEntity(PacmanEntityData &data, ygl::Transformation &transform);
	void go_to_target(int **distanceMap, Direction dir, glm::ivec2 position, PacmanEntityData &data, int dist);
	void run_from_target(int **distanceMap, Direction dir, glm::ivec2 position, PacmanEntityData &data, int dist);
	void resolveAIState(int **map, glm::ivec2 position, unsigned char start, PacmanEntityData &data,
						std::function<void(int **, Direction, glm::ivec2, PacmanEntityData &, int)> tryDirection);
	void ghostAI(ygl::Entity e, ygl::Transformation &transform, PacmanEntityData &data);
	void fillDistanceMap(int **distanceMap, glm::ivec2 start);
	void eatDot(glm::ivec2 position);

	void updatePlayer(ygl::Transformation &transform, PacmanEntityData &data);
	void checkCollision(ygl::Entity ghost, ygl::Transformation &ghostTransform, PacmanEntityData &ghostData,
						ygl::Transformation &pacmanTransform, PacmanEntityData &pacmanData);
	void checkPillTimer();
	void restartAfterDeath();

   public:
	static glm::ivec2 getMapVector(Direction dir);
	static glm::vec2  getWorldVector(Direction dir);

	static const char *name;

	PacmanGame(ygl::Scene *scene, const std::string &map_file, std::size_t width, std::size_t height);

	void init() override;

	void doWork() override;

	~PacmanGame() override;

	unsigned int getScore() { return score; }
	bool		 hasGameEnded() { return gameEnded; }
	bool		 hasGameStarted() { return gameStarted; }
	bool		 isGameWon() { return gameFinishedWin; }
	unsigned int getLives() { return lives; }
	std::size_t	 getWidth() { return width; }
	std::size_t	 getHeight() { return height; }

	void drawGUI();

	void write(std::ostream &out) override;
	void read(std::istream &in) override;
};
