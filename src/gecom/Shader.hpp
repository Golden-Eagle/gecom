/*
 * GECOM Shader Manager
 *
 * Extra preprocessor directives:
 *	- #include "..."  : include file relative to directory containing current file
 *	- #include <...>  : include file relative to directories known to the shader manager
 *      #include resolves #version directives. Failed #include directives are replaced
 *      with #error directives, and as such can safely be used with #if etc.
 *      Malformed #include directives will be replaced with #error directives.
 *  - #shader stage   : specify shader stage(s) source should be compiled for
 *	    Valid values for stage are:
 *	     - vertex
 *       - tess_control
 *       - tess_evaluation
 *       - geometry
 *       - fragment
 *       - compute
 *      Use of #shader with #if etc is supported. Malformed #shader directives will be
 *      replaced with #error directives.
 *
 * The following macros are defined only when compiling for the corresponding shader stage:
 *  - _VERTEX_
 *  - _TESS_CONTROL_
 *  - _TESS_EVALUATION_
 *  - _GEOMETRY_
 *  - _FRAGMENT_
 *  - _COMPUTE_
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

	class shader_error : public std::runtime_error {
	public:
		explicit shader_error(std::string what_ = "shader error") : std::runtime_error(what_) { }
	};

	class shader_compile_error : public shader_error {
	public:
		explicit shader_compile_error(const std::string &what_ = "shader compilation failed") : shader_error(what_) { }
	};

	class shader_link_error : public shader_error {
	public:
		explicit shader_link_error(const std::string &what_ = "shader program linking failed") : shader_error(what_) { }
	};



}

#endif // GECOM_SHADER_HPP
