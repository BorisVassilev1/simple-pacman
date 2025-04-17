#include <yoghurtgl.h>

#include <window.h>
#include <input.h>
#include <ecs.h>
#include <renderer.h>
#include <camera.h>
#include <entities.h>
#include <shader.h>
#include <asset_manager.h>
#include "game/pacman-game.h"

#include <glm/gtx/string_cast.hpp>

using namespace std;

void run() {
	// create window
	ygl::Window window = ygl::Window(600, 800, "Test Window", true, false);

	// create scene and systems
	ygl::Scene scene;
	ygl::Renderer	  *renderer = scene.registerSystem<ygl::Renderer>(&window);
	ygl::AssetManager *asman	= scene.getSystem<ygl::AssetManager>();
	PacmanGame *game = scene.registerSystem<PacmanGame>(std::string("./resources/map.txt"), 21, 22);

	// default shader for the scene
	ygl::VFShader *defaultShader = new ygl::VFShader("./shaders/unlit.vs", "./shaders/unlit.fs");
	renderer->setDefaultShader(asman->addShader(defaultShader, "default_shader"));

	// camera setup
	ygl::OrthographicCamera cam(game->getWidth(), window, 0.01f, 10, ygl::Transformation(glm::vec3(0, 0, 1)));
	renderer->setMainCamera(&cam);
	cam.update();

	// send material data to GPU
	renderer->loadData();

	// main game loop
	glClearColor(1.0f, 0.0f, 0.0f, 1.0);
	while (!window.shouldClose()) {
		window.beginFrame();

		game->doWork();
		renderer->doWork();
		
		game->drawGUI();

		window.swapBuffers();
	}
}

int main() {
	if (ygl::init()) {
		dbLog(ygl::LOG_ERROR, "ygl failed to init");
		exit(1);
	}

	srand(time(NULL));

	run();

	ygl::terminate();
	std::cerr << std::endl;
	return 0;
}
