

#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>

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

	{
		ofstream ofs("☺.txt");
		ofs << "hello world!" << endl;
	}

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	

	while (!win->shouldClose()) {
		Window::pollEvents();
		
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

	delete win;
}
