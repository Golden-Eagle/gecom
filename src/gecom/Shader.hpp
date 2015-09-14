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

#include <cassert>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <utility>
#include <type_traits>

#include "Util.hpp"
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

	class ShaderManager;

	class shader_spec {
		friend class ShaderManager;
	private:
		std::vector<std::string> m_files;
		std::vector<std::string> m_texts;
		std::unordered_map<std::string, std::string> m_defines;
		mutable ShaderManager *m_shaderman = nullptr;
		mutable GLuint m_prog = 0;
		mutable std::chrono::steady_clock::time_point m_timestamp;

	public:
		shader_spec() { }

		shader_spec(const shader_spec &) = default;
		shader_spec & operator=(const shader_spec &) = default;

		shader_spec(shader_spec &&other) :
			m_files(std::move(other.m_files)),
			m_texts(std::move(other.m_texts)),
			m_defines(std::move(other.m_defines)),
			m_timestamp(other.m_timestamp),
			m_prog(other.m_prog)
		{
			other.m_prog = 0;
		}

		shader_spec & operator=(shader_spec &&other) {
			m_files = std::move(other.m_files);
			m_texts = std::move(other.m_texts);
			m_defines = std::move(other.m_defines);
			m_timestamp = other.m_timestamp;
			m_prog = other.m_prog;
			other.m_prog = 0;
		}

		shader_spec & file(std::string fname) {
			m_files.push_back(std::move(fname));
			return *this;
		}

		shader_spec & text(std::string text) {
			m_texts.push_back(std::move(text));
			return *this;
		}

		shader_spec & define(std::string identifier, std::string tokens) {
			assert(util::isIdentifier(identifier));
			m_defines[std::move(identifier)] = std::move(tokens);
			return *this;
		}

		shader_spec & define(std::string identifier) {
			define(std::move(identifier), "");
			return *this;
		}

		template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
		shader_spec & define(std::string identifier, T val) {
			std::ostringstream oss;
			oss << val << (std::is_signed<T>::value ? "" : "u");
			define(std::move(identifier), oss.str());
			return *this;
		}

		shader_spec & define(std::string identifier, float val) {
			std::ostringstream oss;
			oss << std::setprecision(9) << val;
			define(std::move(identifier), oss.str());
			return *this;
		}

		shader_spec & define(std::string identifier, double val) {
			std::ostringstream oss;
			oss << std::setprecision(17) << val << "lf";
			define(std::move(identifier), oss.str());
			return *this;
		}

		const auto & files() const {
			return m_files;
		}

		const auto & texts() const {
			return m_texts;
		}

		const auto & defines() const {
			return m_defines;
		}

		auto & files() {
			return m_files;
		}

		auto & texts() {
			return m_texts;
		}

		auto & defines() {
			return m_defines;
		}

		inline bool operator==(const shader_spec &other) const {
			return m_files == other.m_files && m_texts == other.m_texts && m_defines == other.m_defines;
		}

		inline bool operator!=(const shader_spec &other) const {
			return !(*this == other);
		}
	};

	struct shader_spec_hash {
		size_t operator()(const shader_spec &spec) noexcept {
			// TODO shader_spec hash function
		}
	};

}

#endif // GECOM_SHADER_HPP
