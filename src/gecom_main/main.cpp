

#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>

#include <gecom/Terminal.hpp>
#include <gecom/Window.hpp>
#include <gecom/Log.hpp>
#include <gecom/Concurrent.hpp>

#include <gecom/Util.hpp>
#include <gecom/Serialization.hpp>

using namespace gecom;
using namespace std;

using namespace std::literals;

int main() {
	section_guard sec("main");

	Log::info().verbosity(0) << "Starting...";


	Log::info() << "cpu endianness: " << cpuEndian();
	Log::info() << "fpu endianness: " << fpuEndian();

	string s = "helloooo\4ooo wooooooo\6ooooooo\5\5\5orld!";
	Log::info() << util::hexdumpc(s);




	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	

	while (!win->shouldClose()) {
		Window::pollEvents();
		
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

	delete win;
}
