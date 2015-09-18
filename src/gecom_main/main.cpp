

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

	istringstream iss(ss.str());
	deserializer d(iss.rdbuf());

	int x1;
	double x2;
	d >> x1 >> x2;

	Log::info() << x1 << ", " << setprecision(9) << x2;


	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	

	while (!win->shouldClose()) {
		Window::pollEvents();
		
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

	delete win;
}
