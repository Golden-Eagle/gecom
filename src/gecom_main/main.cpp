

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


	while (!win->shouldClose()) {
		glfwPollEvents();
		if (win->testKey(GLFW_KEY_SPACE)) {
			Log::info() << "SPACEBAR!!!";
			Log::info() << win->mousePosition().x << ", " << win->mousePosition().y;
		}
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

}
