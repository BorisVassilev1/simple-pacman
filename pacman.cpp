#include <yoghurtgl.h>


#include <window.h>
#include <input.h>
#include <ecs.h>
#include <renderer.h>
#include <camera.h>
#include <entities.h>
#include <shader.h>
#include "game/pacman-game.h"

#include <glm/gtx/string_cast.hpp>

using namespace std;

void run() {
	ygl::Window window = ygl::Window(600, 800, "Test Window", true, false);
	ygl::Mouse mouse(window);
	
	ygl::Scene scene;

	ygl::Renderer *renderer = scene.registerSystem<ygl::Renderer>(&window);
	ygl::AssetManager *asman = scene.getSystem<ygl::AssetManager>();
	
	PacmanGame *game = scene.registerSystem<PacmanGame>(std::string("./map.txt"), 21, 22);
	
	ygl::VFShader *defaultShader = new ygl::VFShader("./shaders/unlit.vs", "./shaders/unlit.fs");
	renderer->setDefaultShader(asman->addShader(defaultShader, "default_shader"));

	ygl::OrthographicCamera cam(21, window, 0.01, 10, ygl::Transformation(glm::vec3(0,0,1)));
	cam.update();

	renderer->loadData();

	renderer->getScreenEffect(0)->enabled = false;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0);
	while (!window.shouldClose()) {
		window.beginFrame();
		mouse.update();

		game->doWork();
		renderer->doWork();
		
		window.swapBuffers();
	}
}


int main()
{
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
