# GLAER
###### _OpenGL API Entrypoint Retriever_

GLAER is a C wrapper for the OpenGL API that loads entrypoints at runtime. Source files are generated inside a CMake project with a Python script that parses the OpenGL XML documentation. The script inlines documentation in the generated header if a supported IDE (currently only Visual Studio) is detected.

This is currently _alpha_ software; it has only just reached the point where things actually work.

## OpenGL XML Documentation

The primary API specification can be updated with
`svn co --username anonymous --password anonymous https://cvs.khronos.org/svn/repos/ogl/trunk/doc/registry/public/api/ api` from the repository root.

The function documentation can be updated by first nuking the `/docs` directory, then running `./getdocs.py`.

## Python

The scripts currently run under (and the CMake project looks for) Python 2.7.

The package [Beautiful Soup 4](http://www.crummy.com/software/BeautifulSoup/) is required and is included in GLAER for convenience.

## Test Project

The directory `/test` contains a CMake project (using [GLFW](http://www.glfw.org/)) that does some basic drawing with OpenGL in order to test that entrypoints are being loaded correctly. It can also serve as an example of how to add GLAER as a CMake sub-project.
