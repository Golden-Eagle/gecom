
# gecom sources
set(sources
	"GL.hpp"
	"Initial3D.hpp"
	"Uncopyable.hpp"
	"Util.hpp"
	"Concurrent.hpp"
	"Concurrent.cpp"
	"Shader.hpp"
	"Shader.cpp"
	"SimpleShader.hpp"
	"Perlin.hpp"
	"Perlin.cpp"
	"Window.hpp"
	"Window.cpp"
	"Profiler.hpp"
	"Log.hpp"
	"Log.cpp"
	"Section.hpp"
	"Section.cpp"
	"Terminal.hpp"
	"Terminal.cpp"
	"Platform.hpp"
	"Platform.cpp"
	"Serialization.hpp"
	"Intrinsics.hpp"
)

add_library(gecom ${sources})

source_group("Sources" FILES ${sources})

# Link usage requirements
target_link_libraries(gecom PUBLIC glfw glaer)

# Include usage requirements
# TODO this isn't very nice
target_include_directories(gecom INTERFACE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
)

# Compiler option usage requirements
# not debug: enable intrinsics
gecom_target_msvc_compile_options(gecom PUBLIC "$<$<NOT:$<CONFIG:Debug>>:/Oi>")

