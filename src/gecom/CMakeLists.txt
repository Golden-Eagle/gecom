
# list .hpp
set(headers
	"Concurrent.hpp"
	"GECom.hpp"
	"GL.hpp"
	"Image.hpp"
	"Initial3D.hpp"
	"Log.hpp"
	"Perlin.hpp"
	"Shader.hpp"
	"SimpleShader.hpp"
	"Window.hpp"
)

# list .cpp
set(sources
	"Concurrent.cpp"
	"Log.cpp"
	"Perlin.cpp"
	"Window.cpp"
)

add_library(gecom ${headers} ${sources})
target_link_libraries(gecom PUBLIC glfw glaer)
target_include_directories(gecom INTERFACE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
)
