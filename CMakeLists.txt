
# require new behaviour of:
# CMP0054 (don't dereference quoted variables in if() args)
cmake_minimum_required(VERSION 3.1)

project("GECOM" CXX C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# enable IDE project folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# GLFW
add_subdirectory("${PROJECT_SOURCE_DIR}/ext/glfw-latest")

# GLAER
add_subdirectory("${PROJECT_SOURCE_DIR}/ext/glaer")

# Set GECOM default compile options
include(GEComCompileOptions)
gecom_add_default_compile_options()

# gecom lib target
add_subdirectory(src/gecom)
set_property(TARGET gecom PROPERTY FOLDER "GECOM")

# test exe target
add_subdirectory(src/gecom_main)
set_property(TARGET gecom_main PROPERTY FOLDER "GECOM")
