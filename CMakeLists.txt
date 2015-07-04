cmake_minimum_required(VERSION 3.0.0)

PROJECT("GECOM" CXX C)

# enable IDE project folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# GLFW
add_subdirectory("${PROJECT_SOURCE_DIR}/ext/glfw-latest")

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS ON)
set(GLFW_INSTALL OFF)

# GLAER
add_subdirectory("${PROJECT_SOURCE_DIR}/ext/glaer")

# Set Compiler Flags
if(MSVC)
	# MSVC / Visual Studio
	# Force to always compile with /W4 (Full warnings)
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# GCC
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -Werror=return-type -pedantic -std=c++1y -g")
	# Turn on optimization in release mode
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
	# If GCC >= 4.9, enable coloured output
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
	if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# Clang
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -Werror=return-type -pedantic -std=c++1y -g")
	# Turn on optimization in release mode
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# gecom lib target
add_subdirectory(src/gecom)
set_property(TARGET gecom PROPERTY FOLDER "GECOM")

# test exe target
add_subdirectory(src/main)
set_property(TARGET main PROPERTY FOLDER "GECOM")
