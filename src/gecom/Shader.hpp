/*
 * GECom Shader Manager
 *
 * Extra preprocessor directives:
 *	- #include "..."  : include file relative to directory containing current file
 *	- #include <...>  : include file relative to directories known to the shader manager
 *      #include resolves #version directives.
 *      Failed #include directives are replaced with #error directives.
 *
 *  - #shader type    : specify shader type(s) a file should be compiled as
 *	    Valid values for type are:
 *	     - vertex
 *       - fragment
 *       - geometry
 *       - tess_control
 *       - tess_evaluation
 *
 * The line numbers reported in compiler messages should be correct provided the compiler
 * follows the GLSL spec for the version in question regarding the #line directive.
 * The spec changed regarding this with GLSL 330 (to the best of my knowledge).
 * If the compiler uses the pre-330 behaviour for 330 or later code, line numbers will
 * be reported as 1 greater than they should be.
 *
 */

#ifndef GECOM_SHADER_HPP
#define GECOM_SHADER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>

#include "Uncopyable.hpp"
#include "GL.hpp"

// we only include this to ensure the log is initialized before shader usage
#include "Log.hpp"

namespace gecom {

	

}

#endif // GECOM_SHADER_HPP
