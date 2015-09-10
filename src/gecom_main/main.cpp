

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

	Log::info().verbosity(0) << "Starting...";

	assertMainThread();


	async::invoke(0s, [] {
		this_thread::sleep_for(5s);
		Log::info() << "ASYNC!";
	});


	
	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	auto sub = win->onKeyPress.subscribe([](const key_event &e) {
		Log::info("Key") << e.window;
		return false;
	});


	while (!win->shouldClose()) {
		glfwPollEvents();
		if (win->getKey(GLFW_KEY_SPACE)) {
			Log::info() << "SPACEBAR!!!";
		}
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

	glfwTerminate();

}
