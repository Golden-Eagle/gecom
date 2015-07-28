
#include <gecom/Window.hpp>
#include <gecom/Log.hpp>
#include <gecom/Concurrent.hpp>

using namespace gecom;

int main() {
	Section sec("main");

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
