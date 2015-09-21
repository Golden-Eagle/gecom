

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

	
	string_serializer ss;
	ss << 9001 << 9001.999;
	ss << "Hello world!";

	string_deserializer sd(ss.str());

	int x1;
	double x2;
	string x3;
	sd >> x1 >> x2 >> x3;

	Log::info() << x1 << ", " << setprecision(9) << x2 << ", " << x3 << '|';


	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	

	while (!win->shouldClose()) {
		Window::pollEvents();
		
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

	delete win;
}
