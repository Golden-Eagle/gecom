

#include <thread>
#include <chrono>
#include <iostream>

#include <gecom/Terminal.hpp>
#include <gecom/Window.hpp>
#include <gecom/Log.hpp>
#include <gecom/Concurrent.hpp>

using namespace gecom;
using namespace std;

int main() {
	Section sec("main");

	// need to reference something from terminal.cpp to initialize redirection
	cout << terminal::reset << std::endl;

	fprintf(stdout, "<thisis:stdout>\n");
	fflush(stdout);

	cout << terminal::white << terminal::onRed;
	cerr << terminal::white << terminal::onBlue;

	for (int i = 0; i < 50; i++) {
		cerr << "<thisis:cerr>" << endl;
		cout << "<thisis:cout>" << endl;
		this_thread::sleep_for(chrono::milliseconds(50));
	}


	AsyncExecutor::start();

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).visible(true);

	win->makeContextCurrent();

	{
		Section sec("foo");
		Log::warning() << "this is a warning";
		{
			Section sec("foo");
			Log::warning() << "this is still a warning";
			{
				Section sec("foo");
				Log::warning() << "this is just a warning";
				{
					Section sec("foo");
					Log::warning() << "this used to be a warning, but we can't afford them anymore";
					{
						Section sec("bar");
						Log::error() << "this is an error, fix it";
					}
				}
			}
		}
	}

	while (!win->shouldClose()) {
		glfwPollEvents();
		win->swapBuffers();
	}

	glfwTerminate();

	AsyncExecutor::stop();
}
