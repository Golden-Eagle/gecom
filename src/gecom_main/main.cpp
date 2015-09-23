

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

	file_deserializer fd("./blah.bin");

	if (fd.is_open()) {
		unordered_map<int32_t, string> m;
		fd >> m;

		for (const auto &e : m) {
			Log::info() << e.first << " : " << e.second;
		}

		bitset<14> s;
		fd >> s;
		bitset<70> ss;
		fd >> ss;

		Log::info() << s.to_string();
		Log::info() << ss.to_string();
	}

	fd.close();

	unordered_map<int32_t, string> m {
		{ 1, "buddy" },
		{ 2, "you're" },
		{ 3, "a" },
		{ 4, "boy" },
		{ 5, "make" },
		{ 6, "a" },
		{ 7, "big" },
		{ 8, "noise" },
		{ 9, "playing" },
		{ 10, string(256, '-') }
	};
	

	file_serializer fs("./blah.bin");

	fs << m;


	bitset<14> s(0b11001010101001);
	fs << s;
	bitset<70> ss(ULLONG_MAX - 1);
	fs << ss;

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	win->makeCurrent();

	

	while (!win->shouldClose()) {
		Window::pollEvents();
		
		win->swapBuffers();
		this_thread::sleep_for(5ms);
	}

	delete win;
}
