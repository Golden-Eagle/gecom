
/*
 * GECOM Platform Helpers
 *
 * Platform detection macros, platform-specific functionality and
 * cross-platform wrappers for common functionality.
 */

#ifndef GECOM_PLATFORM_HPP
#define GECOM_PLATFORM_HPP

#include <string>
#include <exception>

// static init dependency
#include "Section.hpp"

#ifdef _WIN32
#define GECOM_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// test for toolchains that support functions for manipulation of FILE pointers,
// like _fileno, _get_osfhandle, _fdopen, _open_osfhandle
#define GECOM_FOUND_MSVCRT_STDIO
#endif

namespace gecom {

	namespace platform {

		class win32_error : public std::exception {
		private:
			int m_err;
			std::string m_what;

		public:
			win32_error(int err_, const std::string &hint_ = "");

			virtual const char * what() const noexcept override;

			int code() const noexcept {
				return m_err;
			}
		};

	}
}

#endif

#if defined(__APPLE__)
#define GECOM_PLATFORM_POSIX
#include "TargetConditionals.h"
#if TARGET_OS_MAC
#define GECOM_PLATFORM_MAC
#endif
#endif

#if defined(__unix__)
// TODO improve detection
#define GECOM_PLATFORM_POSIX
#endif

namespace gecom {

	class PlatformInit {
	private:
		static size_t refcount;
	public:
		PlatformInit();
		~PlatformInit();
	};

	namespace platform {

		class module_handle {
		private:
			void *m_hmod = nullptr;

			void destroy() noexcept;

		public:
			module_handle() { }

			explicit module_handle(void *hmod_) : m_hmod(hmod_) { }

			explicit module_handle(const std::string &modname_);

			module_handle(const module_handle &) = delete;
			module_handle & operator=(const module_handle &) = delete;

			module_handle(module_handle &&other) noexcept : m_hmod(other.m_hmod) {
				other.m_hmod = nullptr;
			}

			module_handle & operator=(module_handle &&other) noexcept {
				destroy();
				m_hmod = other.m_hmod;
				other.m_hmod = nullptr;
				return *this;
			}

			void * nativeHandle() const {
				return m_hmod;
			}

			void detach() {
				m_hmod = nullptr;
			}

			void * procAddress(const std::string &procname) const;

			bool operator==(const module_handle &rhs) const {
				return m_hmod == rhs.m_hmod;
			}

			bool operator!=(const module_handle &rhs) const {
				return !(*this == rhs);
			}

			~module_handle() {
				destroy();
			}
		};

		void throwLastError(const std::string &hint = "");

		void hookImportedProc(const std::string &modname, const std::string &procname, const void *newproc, const void **oldproc);

		template <typename T>
		void hookImportedProc(const std::string &modname, const std::string &procname, const void *newproc, T **oldproc) {
			// for convenience
			hookImportedProc(modname, procname, newproc, reinterpret_cast<const void **>(oldproc));
		}

	}
}

namespace {
	gecom::PlatformInit platform_init_obj;
}

#endif // GECOM_PLATFORM_HPP
