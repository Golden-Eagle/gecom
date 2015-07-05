
#include <gecom/Window.hpp>

using namespace gecom;

int main() {

	gecom::Window *win = createWindow().title("Hello World").size(640, 480).visible(true);

	while (!win->shouldClose()) {
		glfwPollEvents();
		win->swapBuffers();
	}

	glfwTerminate();

}
