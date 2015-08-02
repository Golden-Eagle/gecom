
#include "Platform.hpp"

#ifdef GECOM_PLATFORM_WIN32

#include <windows.h>

namespace gecom {

	win32_error::win32_error(int err_, const std::string &hint_) : m_err(err_) {
		char buf[256];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);
		buf[sizeof(buf) - 1] = '\0';
		if (!hint_.empty()) {
			m_what += hint_;
			m_what += ": ";
		}
		m_what += buf;
	}

	const char * win32_error::what() const noexcept {
		return &m_what.front();
	}

	void throwLastWin32Error(const std::string & hint) {
		throw win32_error(GetLastError(), hint);
	}

}

#endif // GECOM_PLATFORM_WIN32
