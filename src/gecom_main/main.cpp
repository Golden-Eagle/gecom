

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


	async::invoke(0s, [] {
		this_thread::sleep_for(5s);
		Log::info() << "ASYNC!";
	});


	
	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	auto sub = win->onKeyPress.subscribe([](const key_event &e) {
		Log::info("Key") << e.key << ", " << e.euid;
		return false;
	});

	while (!win->shouldClose()) {
		Window::pollEvents();
		if (win->testKey(GLFW_KEY_SPACE)) {
			//Log::info() << "SPACEBAR!!!";
			//Log::info() << win->mousePosition().x << ", " << win->mousePosition().y;
			key_event e;
			e.key = 9001;
			e.action = GLFW_PRESS;
			Window::dispatchGlobalEvent(e);
		}
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

}
