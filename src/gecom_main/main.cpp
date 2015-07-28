
#include <gecom/Window.hpp>
#include <gecom/Log.hpp>

using namespace gecom;

int main() {
	Section sec("main");

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).visible(true);

	while (!win->shouldClose()) {
		glfwPollEvents();
		win->swapBuffers();
	}

	glfwTerminate();

}
