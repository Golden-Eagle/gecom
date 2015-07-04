
#ifndef SKADI_SIMPLE_SHADER_HPP
#define SKADI_SIMPLE_SHADER_HPP

#include <string>
#include <vector>
#include <sstream>

#include "GL.hpp"
#include "Log.hpp"

namespace skadi {


	class shader_error : public std::runtime_error {
	public:
		explicit shader_error(const std::string &what_ = "Generic shader error.") : std::runtime_error(what_) { }
	};

	class shader_type_error : public shader_error {
	public:
		explicit shader_type_error(const std::string &what_ = "Bad shader type.") : shader_error(what_) { }
	};

	class shader_compile_error : public shader_error {
	public:
		explicit shader_compile_error(const std::string &what_ = "Shader compilation failed.") : shader_error(what_) { }
	};

	class shader_link_error : public shader_error {
	public:
		explicit shader_link_error(const std::string &what_ = "Shader program linking failed.") : shader_error(what_) { }
	};

	inline void printShaderInfoLog(GLuint obj) {
		int infologLength = 0;
		int charsWritten = 0;
		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::vector<char> infoLog(infologLength);
			glGetShaderInfoLog(obj, infologLength, &charsWritten, &infoLog[0]);
			gecom::log("SimpleShader") << "SHADER:\n" << &infoLog[0];
		}
	}

	inline void printProgramInfoLog(GLuint obj) {
		int infologLength = 0;
		int charsWritten = 0;
		glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::vector<char> infoLog(infologLength);
			glGetProgramInfoLog(obj, infologLength, &charsWritten, &infoLog[0]);
			gecom::log("SimpleShader") << "PROGRAM:\n" << &infoLog[0];
		}
	}

	inline GLuint compileShader(GLenum type, const std::string &text) {
		GLuint shader = glCreateShader(type);
		const char *text_c = text.c_str();
		glShaderSource(shader, 1, &text_c, nullptr);
		glCompileShader(shader);
		GLint compile_status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
		if (!compile_status) {
			printShaderInfoLog(shader);
			throw shader_compile_error();
		}
		// always print, so we can see warnings
		printShaderInfoLog(shader);
		return shader;
	}

	inline void linkShaderProgram(GLuint prog) {
		glLinkProgram(prog);
		GLint link_status;
		glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
		if (!link_status) {
			printProgramInfoLog(prog);
			throw shader_link_error();
		}
		// always print, so we can see warnings
		printProgramInfoLog(prog);
	}

	inline GLuint makeShaderProgram(const std::string &profile, const std::vector<GLenum> &stypes, const std::string &source) {
		GLuint prog = glCreateProgram();

		auto get_define = [](GLenum stype) {
			switch (stype) {
			case GL_VERTEX_SHADER:
				return "_VERTEX_";
			case GL_GEOMETRY_SHADER:
				return "_GEOMETRY_";
			case GL_TESS_CONTROL_SHADER:
				return "_TESS_CONTROL_";
			case GL_TESS_EVALUATION_SHADER:
				return "_TESS_EVALUATION_";
			case GL_FRAGMENT_SHADER:
				return "_FRAGMENT_";
			default:
				return "_DAMN_AND_BLAST_";
			}
		};

		for (auto stype : stypes) {
			std::ostringstream oss;
			oss << "#version " << profile << std::endl;
			oss << "#define " << get_define(stype) << std::endl;
			oss << source;
			auto shader = compileShader(stype, oss.str());
			glAttachShader(prog, shader);
		}

		linkShaderProgram(prog);
		gecom::log("SimpleShader") << "Shader program compiled and linked successfully";
		return prog;
	}

}

#endif
