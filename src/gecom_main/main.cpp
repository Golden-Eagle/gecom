

#include <cmath>
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


	
	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	auto sub = win->onKeyPress.subscribe([](const key_event &e) {
		Log::info("Key") << e.key << ", " << e.euid;
		return false;
	});


	auto sub2 = win->onJoystick.subscribe([](const joystick_event &e) {
		if (fabs(e.state.axis(0)) > 0.1f || fabs(e.state.axis(1)) > 0.1f) {
			Log::info("Joystick") << e.state.axis(0) << ", " << e.state.axis(1);
		}
		return false;
	});

	auto sub3 = win->onJoystickPresence.subscribe([](const joystick_presence_event &e) {
		Log::info("Joystick") << e.state.name << " [" << e.state.token << "]" << (e.present ? " connected" : " disconnected");
		return false;
	});

	auto sub4 = win->onJoystickButtonPress.subscribe([](const joystick_button_event &e) {
		Log::info("Joystick") << "button " << e.button;
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

	delete win;
}
