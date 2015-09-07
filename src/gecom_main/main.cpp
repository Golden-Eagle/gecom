

#include <thread>
#include <chrono>
#include <iostream>

#include <gecom/Terminal.hpp>
#include <gecom/Window.hpp>
#include <gecom/Log.hpp>
#include <gecom/Concurrent.hpp>

using namespace gecom;
using namespace std;

using namespace std::literals;

int main() {
	section_guard sec("main");

	// this seems to encourage terminal to init before async
	cout << terminal::reset << std::endl;

	Log::info().verbosity(0) << "Starting...";

	assertMainThread();

	
	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	
	while (!win->shouldClose()) {
		glfwPollEvents();
		if (win->getKey(GLFW_KEY_SPACE)) {
			Log::info() << "SPACEBAR!!!";
		}
		win->swapBuffers();
	}

	glfwTerminate();

}
