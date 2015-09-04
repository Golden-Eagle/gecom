
// GECOM platform detection macros
#ifndef GECOM_PLATFORM_HPP
#define GECOM_PLATFORM_HPP

#include <string>
#include <exception>

#ifdef _WIN32
#define GECOM_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// test for toolchains that support functions for manipulation of FILE pointers,
// like _fileno, _get_osfhandle, _fdopen, _open_osfhandle
#define GECOM_CAN_HAVE_MSVCRT_STDIO
#endif

namespace gecom {

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

	void throwLastWin32Error(const std::string &hint = "");
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

#endif // GECOM_PLATFORM_HPP
