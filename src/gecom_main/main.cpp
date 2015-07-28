
#include <gecom/Window.hpp>
#include <gecom/Log.hpp>
#include <gecom/Concurrent.hpp>

using namespace gecom;

int main() {
	Section sec("main");

	AsyncExecutor::start();

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).visible(true);

	win->makeContextCurrent();

	while (!win->shouldClose()) {
		glfwPollEvents();
		win->swapBuffers();
	}

	glfwTerminate();

	AsyncExecutor::stop();
}
