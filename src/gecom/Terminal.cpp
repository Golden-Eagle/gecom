
#include "Platform.hpp"

#ifdef GECOM_PLATFORM_WIN32
#include <windows.h>
#endif

#ifdef GECOM_PLATFORM_POSIX
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <cassert>
#include <cstdio>
#include <vector>

#include "Terminal.hpp"

namespace {

	// stdout/stderr handles, platform specific
	// note that these can be redirected
#ifdef GECOM_PLATFORM_WIN32
	using handle_t = HANDLE;
	handle_t handle_stdout { GetStdHandle(STD_OUTPUT_HANDLE) };
	handle_t handle_stderr { GetStdHandle(STD_ERROR_HANDLE) };
#else
	using handle_t = FILE *;
	handle_t handle_stdout { stdout };
	handle_t handle_stderr { stderr };
#endif

	// streambuf for writing to stdout/stderr
	// http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
	template <typename CharT>
	class basic_stdoutbuf : public std::basic_streambuf<CharT> {
	private:
		handle_t m_handle;
		std::vector<CharT> m_buf;

		bool syncImpl() {

#if defined(GECOM_PLATFORM_WIN32)

#elif defined(GECOM_PLATFORM_POSIX)

#else

#endif
			return true;
		}

		virtual int_type overflow(int_type ch) override {
			if (ch != traits_type::eof()) {
				assert(pptr() <= epptr());
				*pptr() = ch;
				pbump(1);
				if (syncImpl()) return ch;
			}
			return traits_type::eof();
		}

		virtual int sync() override {
			return syncImpl() ? 0 : -1;
		}

	public:
		basic_stdoutbuf(const basic_stdoutbuf &) = delete;
		basic_stdoutbuf & operator=(const basic_stdoutbuf &) = delete;

		explicit basic_stdoutbuf(handle_t h, size_t bufsz_ = 256) :
			m_handle(h),
			m_buf(bufsz_ + 1)
		{
			setp(&m_buf.front(), &m_buf.back() - 1);
		}
	};

	using stdoutbuf = basic_stdoutbuf<char>;
	using wstdoutbuf = basic_stdoutbuf<wchar_t>;

	// actual streambuf objects
	stdoutbuf buf_stdout { handle_stdout };
	stdoutbuf buf_stderr { handle_stderr };
	wstdoutbuf buf_wstdout { handle_stdout };
	wstdoutbuf buf_wstderr { handle_stderr };

}

namespace gecom {

	namespace terminal {

		// color off
		std::ostream & colorOff(std::ostream &o) { o << "\033[0m"; return o; }

		// regular colors
		std::ostream & black(std::ostream &o) { o << "\033[0;30m"; return o; }
		std::ostream & red(std::ostream &o) { o << "\033[0;31m"; return o; }
		std::ostream & green(std::ostream &o) { o << "\033[0;32m"; return o; }
		std::ostream & yellow(std::ostream &o) { o << "\033[0;33m"; return o; }
		std::ostream & blue(std::ostream &o) { o << "\033[0;34m"; return o; }
		std::ostream & purple(std::ostream &o) { o << "\033[0;35m"; return o; }
		std::ostream & cyan(std::ostream &o) { o << "\033[0;36m"; return o; }
		std::ostream & white(std::ostream &o) { o << "\033[0;37m"; return o; }

		// bold colors
		std::ostream & boldBlack(std::ostream &o) { o << "\033[1;30m"; return o; }
		std::ostream & boldRed(std::ostream &o) { o << "\033[1;31m"; return o; }
		std::ostream & boldGreen(std::ostream &o) { o << "\033[1;32m"; return o; }
		std::ostream & boldYellow(std::ostream &o) { o << "\033[1;33m"; return o; }
		std::ostream & boldBlue(std::ostream &o) { o << "\033[1;34m"; return o; }
		std::ostream & boldPurple(std::ostream &o) { o << "\033[1;35m"; return o; }
		std::ostream & boldCyan(std::ostream &o) { o << "\033[1;36m"; return o; }
		std::ostream & boldWhite(std::ostream &o) { o << "\033[1;37m"; return o; }

		std::streambuf * stdoutBuf() {
			return &buf_stdout;
		}

		std::streambuf * stderrBuf() {
			return &buf_stderr;
		}

		std::wstreambuf * wstdoutBuf() {
			return &buf_wstdout;
		}

		std::wstreambuf * wstderrBuf() {
			return &buf_wstderr;
		}

	}
}
