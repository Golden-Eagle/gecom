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

	namespace shader {

		class build_error : public std::runtime_error {
		public:
			explicit build_error(const std::string &what_ = "shader build failed") : std::runtime_error(what_) { }
		};

		class file_not_found : public build_error {
		public:
			explicit file_not_found(const std::string &what_ = "file not found") : build_error(what_) { }
		};

		class compile_error : public build_error {
		public:
			explicit compile_error(const std::string &what_ = "shader compilation failed") : build_error(what_) { }
		};

		class link_error : public build_error {
		public:
			explicit link_error(const std::string &what_ = "shader program linking failed") : build_error(what_) { }
		};

		// shader program specification: source files, source texts, preprocessor defines
		class progspec {
		private:
			std::vector<std::string> m_files;
			std::vector<std::string> m_texts;
			std::unordered_map<std::string, std::string> m_defines;
			mutable GLuint m_prog = 0;
			mutable std::chrono::steady_clock::time_point m_timestamp;

		public:
			progspec() { }

			progspec(const progspec &) = default;
			progspec & operator=(const progspec &) = default;

			progspec(progspec &&other) :
				m_files(std::move(other.m_files)),
				m_texts(std::move(other.m_texts)),
				m_defines(std::move(other.m_defines)),
				m_timestamp(other.m_timestamp),
				m_prog(other.m_prog)
			{
				other.m_prog = 0;
			}

			progspec & operator=(progspec &&other) {
				m_files = std::move(other.m_files);
				m_texts = std::move(other.m_texts);
				m_defines = std::move(other.m_defines);
				m_timestamp = other.m_timestamp;
				m_prog = other.m_prog;
				other.m_prog = 0;
			}

			progspec & file(std::string fname) {
				m_files.push_back(std::move(fname));
				return *this;
			}

			progspec & text(std::string text) {
				m_texts.push_back(std::move(text));
				return *this;
			}

			progspec & define(std::string identifier, std::string tokens) {
				assert(util::isIdentifier(identifier));
				m_defines[std::move(identifier)] = std::move(tokens);
				return *this;
			}

			progspec & define(std::string identifier) {
				define(std::move(identifier), "");
				return *this;
			}

			template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
			progspec & define(std::string identifier, T val) {
				std::ostringstream oss;
				oss << val << (std::is_signed<T>::value ? "" : "u");
				define(std::move(identifier), oss.str());
				return *this;
			}

			progspec & define(std::string identifier, float val) {
				std::ostringstream oss;
				oss << std::setprecision(9) << std::showpoint << val;
				define(std::move(identifier), oss.str());
				return *this;
			}

			progspec & define(std::string identifier, double val) {
				std::ostringstream oss;
				oss << std::setprecision(17) << std::showpoint << val << "lf";
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

			inline bool operator==(const progspec &other) const {
				return m_files == other.m_files && m_texts == other.m_texts && m_defines == other.m_defines;
			}

			inline bool operator!=(const progspec &other) const {
				return !(*this == other);
			}
		};

		struct progspec_hash {
			size_t operator()(const progspec &spec) noexcept {
				// TODO progspec hash function
			}
		};

		// list the absolute final paths of the known source directories
		std::vector<std::string> sourceDirectories();

		// replace the known source directories (searched first to last).
		// throws file_not_found (in that case, the current list is preserved).
		void sourceDirectories(const std::vector<std::string> &);

		// prepend a source directory (searched before current known directories).
		// throws file_not_found (in that case, the current list is preserved).
		void prependSourceDirectory(const std::string &);

		// append a source directory (searched after current known directories)
		// throws file_not_found (in that case, the current list is preserved).
		void appendSourceDirectory(const std::string &);

		// clear the list of known source directories
		void clearSourceDirectories();

		// replace file names with absolute final paths (see loadProgram()). throws file_not_found.
		progspec canonicalize(const progspec &);

		// load a program object (shared with the current context share group), building if a cached object is not present.
		// cached binaries will be loaded instead of building if available and newer than all files they depend on (sources + includes).
		// absolute source file paths will be opened as-is. relative paths will be tested against known source directories
		// in order until a match is found. throws file_not_found, compile_error, link_error.
		GLuint loadProgram(const progspec &);

		// load a program object as in loadProgram(), then attach it to a pipeline object specific to the current context.
		// throws compile_error, link_error.
		GLuint loadPipeline(const progspec &);

		// drop all cached binaries, try rebuild specifications for all loaded programs in current context share group,
		// replace those program objects and return true on success, return false on failure without changing loaded programs.
		bool reloadAll() noexcept;

		// as for reloadAll(), except only outdated binaries are dropped and corresponding programs rebuilt.
		bool reloadChanged() noexcept;
		
	}
}

#endif // GECOM_SHADER_HPP
