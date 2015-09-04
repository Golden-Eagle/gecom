

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

	// need to reference something from terminal.cpp to initialize redirection
	cout << terminal::reset << std::endl;

	// test
	auto fut = async::invoke(1s, [](int a, int b) {
		Log::info() << "ASYNC! : " << (a + b);
		return a + b;
	}, 1, 2);

	Log::info() << "FUTURE! : " << fut.get();

	std::atomic<int> c { 0 };

	for (int i = 0; i < 1000; i++) {
		async::invokeMain(1s, [&] {
			c++;
		});
	}
	
	async::execute(1ms);

	Log::info() << "dsghsfh " << c;

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).contextVersion(4, 1).visible(true);

	Log::info("TEST") << "test...";

	win->makeCurrent();

	{
		section_guard sec("foo");
		Log::warning() << "this is a warning";
		{
			section_guard sec("foo");
			Log::warning() << "this is still a warning";
			{
				section_guard sec("foo");
				Log::warning() << "this is just a warning";
				{
					section_guard sec("foo");
					Log::warning() << "this used to be a warning, but we can't afford them anymore";
					{
						section_guard sec("bar");
						Log::error() << "this is an error, fix it";
					}
				}
			}
		}
	}

	for (int i = 40; i < 60; i++) {
		section_guard sec("overflow");
		string s;
		for (int j = 0; j < i; j++) {
			s += 'x';
		}
		Log::info() << s;
	}

	while (!win->shouldClose()) {
		glfwPollEvents();
		if (win->getKey(GLFW_KEY_SPACE)) {
			Log::info() << "SPACEBAR!!!";
		}
		win->swapBuffers();
	}

	glfwTerminate();

}
