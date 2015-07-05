cmake_minimum_required(VERSION 3.0.0)

project("GECOM" CXX C)

# enable IDE project folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# GLFW
add_subdirectory("${PROJECT_SOURCE_DIR}/ext/glfw-latest")

# GLAER
add_subdirectory("${PROJECT_SOURCE_DIR}/ext/glaer")

# Set GECOM default compile options
include(CompileOptions.cmake)
gecom_add_default_compile_options()

# gecom lib target
add_subdirectory(src/gecom)
set_property(TARGET gecom PROPERTY FOLDER "GECOM")

# test exe target
add_subdirectory(src/gecom_main)
set_property(TARGET gecom_main PROPERTY FOLDER "GECOM")
