
#include "Platform.hpp"

#ifdef GECOM_PLATFORM_WIN32
#include <windows.h>
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// incomplete test for toolchains that support the required functions
#define GECOM_STDIO_REDIRECT_WIN32
#include <io.h>
#include <fcntl.h>
#endif
#endif

#ifdef GECOM_PLATFORM_POSIX
#define GECOM_STDIO_REDIRECT_POSIX
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <memory>
#include <utility>

#include "Terminal.hpp"

namespace {

	template <typename TextFunT, typename EscapeFunT>
	char * processEscapeSequences(char *pbegin, size_t count, const TextFunT &textfun, const EscapeFunT &escapefun) {
		// TODO deal with escape sequences larger than buffer
		bool isescape = false;
		char *pprev = pbegin;
		// find and deal with escape sequences and text sequences, except for last char
		for (char *p = pbegin; p < pbegin + count - 1; p++) {
			if (!isescape && p[0] == '\033' && p[1] == '[') {
				// 'escape' '[' marks start of escape sequence
				isescape = true;
				if (p > pprev) {
					textfun(pprev, p - pprev);
					pprev = p;
				}
			} else if (isescape && std::isalpha(*p)) {
				// alpha char marks end of escape sequence
				isescape = false;
				escapefun(pprev, p - pprev + 1);
				pprev = p + 1;
			}
		}
		// remaining data
		size_t bytesremaining = pbegin + count - pprev;
		char *plast = pbegin + count - 1;
		if (isescape) {
			if (std::isalpha(*plast)) {
				// escape sequence is actually complete
				escapefun(pprev, bytesremaining);
				// next write to start of buffer
				return pbegin;
			} else {
				// move incomplete escape sequence to start of buffer
				memmove(pbegin, pprev, bytesremaining);
				// next write to end of incomplete sequence
				return pbegin + bytesremaining;
			}
		} else {
			if (*plast == '\033') {
				// last char could be start of escape sequence
				if (bytesremaining > 1) textfun(pprev, bytesremaining - 1);
				// move escape to start of buffer
				*pbegin = '\033';
				// next write to after leading escape
				return pbegin + 1;
			} else {
				// safe to write last char too
				if (bytesremaining) textfun(pprev, bytesremaining);
				// next write to start of buffer
				return pbegin;
			}
		}
	}

#if defined(GECOM_STDIO_REDIRECT_WIN32)
#define GECOM_TERMINAL_ANSI

	using gecom::throwLastWin32Error;
	using gecom::win32_error;

	// worker thread
	std::thread redirect_worker;

	std::string makeRandomPipeName() {
		std::mt19937 rand { std::random_device()() };
		std::string name = "\\\\.\\pipe\\gecom";
		const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
		std::uniform_int_distribution<int> dist(0, sizeof(chars) - 2);
		for (int i = 0; i < 30; i++) {
			name += chars[dist(rand)];
		}
		return name;
	}

	class Redirection {
	private:
		static constexpr size_t internalbufsize = 256;
		enum class pipe_state { connecting, reading };
		FILE *m_fp;
		std::string m_pipename;
		HANDLE m_hreal;
		HANDLE m_hpiper;
		OVERLAPPED m_overlap;
		std::vector<char> m_buf;
		char *m_pwrite;
		pipe_state m_state = pipe_state::connecting;
		bool m_pendingio = false;
		bool m_isatty = false;
		CONSOLE_SCREEN_BUFFER_INFO m_csbi_org;
		CONSOLE_SCREEN_BUFFER_INFO m_csbi_real;

	public:
		Redirection(const Redirection &) = delete;
		Redirection & operator=(const Redirection &) = delete;

		Redirection(FILE *fp_, size_t pipebufsize_) : m_fp(fp_), m_buf(internalbufsize, 0), m_pipename(makeRandomPipeName()) {
			
			// initialize write buffer pointer
			m_pwrite = &m_buf.front();

			// initialize overlap struct
			m_overlap.Offset = 0;
			m_overlap.OffsetHigh = 0;
			m_overlap.Pointer = nullptr;

			// create overlapped io event
			// event must remain signaled except when there is a pending operation
			m_overlap.hEvent = CreateEvent(nullptr, true, true, nullptr);
			if (m_overlap.hEvent == INVALID_HANDLE_VALUE) {
				throwLastWin32Error("overlapped io event create");
			}

			// make named pipe in overlapped mode
			m_hpiper = CreateNamedPipe(
				m_pipename.c_str(),
				PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				1,
				pipebufsize_,
				pipebufsize_,
				0,
				nullptr
			);
			if (m_hpiper == INVALID_HANDLE_VALUE) {
				throwLastWin32Error("named pipe create");
			}

			// begin connect server end of pipe
			if (ConnectNamedPipe(m_hpiper, &m_overlap)) {
				// should not happen ever
				throwLastWin32Error("named pipe connect");
			}

			// update state
			switch (GetLastError()) {
			case ERROR_IO_PENDING:
				m_state = pipe_state::connecting;
				m_pendingio = true;
				break;
			case ERROR_PIPE_CONNECTED:
				m_state = pipe_state::reading;
				m_pendingio = false;
				if (!SetEvent(m_overlap.hEvent)) {
					throwLastWin32Error("pipe event set");
				}
				break;
			default:
				throwLastWin32Error("named pipe connect");
				break;
			}

			// get real handle duplicate
			// freopen causes the original to be closed and the win32 std handle to be updated
			DuplicateHandle(
				GetCurrentProcess(),
				HANDLE(_get_osfhandle(_fileno(m_fp))),
				GetCurrentProcess(),
				&m_hreal,
				0,
				false,
				DUPLICATE_SAME_ACCESS
			);

			// reopen FILE to point to our named pipe
			if (!freopen(m_pipename.c_str(), "w", m_fp)) {
				throwLastWin32Error("freopen named pipe");
			}

			// get original console screen buffer info, determine if we're a console
			if (GetConsoleScreenBufferInfo(m_hreal, &m_csbi_org)) {
				m_isatty = true;
				m_csbi_real = m_csbi_org;
			}

		}

		HANDLE eventHandle() const noexcept {
			return m_overlap.hEvent;
		}

		// returns true on success, false if previous operation was cancelled
		bool onEventSignaled() {
			// process result of overlapped operation
			if (m_pendingio) {
				m_pendingio = false;
				DWORD bytesread = 0;
				bool success = GetOverlappedResult(m_hpiper, &m_overlap, &bytesread, false);
				DWORD err = GetLastError();
				if (err == ERROR_OPERATION_ABORTED) {
					// if cancelled, don't start another operation
					// ensure we don't continue unless asked
					ResetEvent(m_overlap.hEvent);
					return false;
				}
				switch (m_state) {
				case pipe_state::connecting:
					// pending connect
					if (!success) {
						close();
						throw win32_error(err, "overlapped named pipe connect");
					}
					m_state = pipe_state::reading;
					break;
				case pipe_state::reading:
					// pending read
					if (!success) {
						close();
						throw win32_error(err, "overlapped named pipe read");
					}
					m_pwrite += bytesread;
					flush();
					break;
				default:
					break;
				}
			}
			// begin next operation
			if (m_state == pipe_state::reading) {
				DWORD bytesread = 0;
				// readfile clears the event if read is pending
				if (ReadFile(
					m_hpiper,
					m_pwrite,
					&m_buf.front() + m_buf.size() - m_pwrite,
					&bytesread,
					&m_overlap
				)) {
					// read completed successfully
					m_pwrite += bytesread;
					flush();
					// ensure we can continue
					SetEvent(m_overlap.hEvent);
				} else if (GetLastError() == ERROR_IO_PENDING) {
					// read is pending
					m_pendingio = true;
				} else {
					// read broke
					close();
					throwLastWin32Error("named pipe read");
				}
			}
			return true;
		}

		void cancel() noexcept {
			CancelIoEx(m_hpiper, &m_overlap);
			//fprintf(stderr, "%s\n", win32_error(GetLastError(), "cancel").what());
		}

		void close() noexcept {
			if (m_hreal != INVALID_HANDLE_VALUE) {
				// cancel pending io
				cancel();
				// attempt to read and flush any available data
				DWORD bytesavail = 0;
				do {
					// update available byte count
					if (!PeekNamedPipe(
						m_hpiper,
						m_pwrite,
						&m_buf.front() + m_buf.size() - m_pwrite,
						nullptr,
						&bytesavail,
						nullptr
					)) {
						// discard errors
						break;
					}
					if (bytesavail > 0) {
						// read bytes synchronously
						DWORD bytesread = 0;
						if (!ReadFile(
							m_hpiper,
							m_pwrite,
							bytesavail,
							&bytesread,
							nullptr
						)) {
							// discard errors
							break;
						}
						// push bytes through
						try {
							flush();
						} catch (...) {
							// discard errors
							break;
						}
					}
				} while (bytesavail > 0);
				// disconnect
				// TODO if there was pending io, does disconnecting make it safe to delete the OVERLAPPED object?
				DisconnectNamedPipe(m_hpiper);
				// close handles
				CloseHandle(m_overlap.hEvent);
				CloseHandle(m_hpiper);
				CloseHandle(m_hreal);
				m_hpiper = INVALID_HANDLE_VALUE;
				m_hreal = INVALID_HANDLE_VALUE;
			}
		}

		void handleText(const char *pbegin, size_t count) {
			assert(count <= m_buf.size());
			// if we're a console, set text attributes
			if (m_isatty) SetConsoleTextAttribute(m_hreal, m_csbi_real.wAttributes);
			// write text
			DWORD byteswritten = 0;
			// TODO this blocked on me at one point
			if (!WriteFile(m_hreal, pbegin, DWORD(count), &byteswritten, nullptr)) {
				throwLastWin32Error("write");
			}
		}

		void handleEscape(const char *pbegin, size_t count) {
			assert(count <= m_buf.size());
			// parse escape code if we're a console
			if (m_isatty) {
				char esctype = *(pbegin + count - 1);
				if (esctype = 'm') {
					// update text attributes
					int code = 0;
					for (const char *p = pbegin; p < pbegin + count; p++) {
						if (std::isdigit(*p)) {
							code = code * 10 + *p - '0';
						} else {
							switch (code) {
							case 0: // reset
								m_csbi_real.wAttributes = m_csbi_org.wAttributes;
								break;
							case 1: // bold
								m_csbi_real.wAttributes |= FOREGROUND_INTENSITY;
								break;
							case 4: // underscore
								m_csbi_real.wAttributes |= COMMON_LVB_UNDERSCORE;
								break;
							case 7: // reverse video
								m_csbi_real.wAttributes |= COMMON_LVB_REVERSE_VIDEO;
							case 30: // foreground black
							case 31: // foreground red
							case 32: // foreground green
							case 33: // foreground yellow
							case 34: // foreground blue
							case 35: // foreground magenta
							case 36: // foreground cyan
							case 37: // foreground white
								m_csbi_real.wAttributes &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
								m_csbi_real.wAttributes |= ((code - 30) & 0x1) ? FOREGROUND_RED : 0;
								m_csbi_real.wAttributes |= ((code - 30) & 0x2) ? FOREGROUND_GREEN : 0;
								m_csbi_real.wAttributes |= ((code - 30) & 0x4) ? FOREGROUND_BLUE : 0;
								break;
							case 40: // background black
							case 41: // background red
							case 42: // background green
							case 43: // background yellow
							case 44: // background blue
							case 45: // background magenta
							case 46: // background cyan
							case 47: // background white
								m_csbi_real.wAttributes &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
								m_csbi_real.wAttributes |= ((code - 40) & 0x1) ? BACKGROUND_RED : 0;
								m_csbi_real.wAttributes |= ((code - 40) & 0x2) ? BACKGROUND_GREEN : 0;
								m_csbi_real.wAttributes |= ((code - 40) & 0x4) ? BACKGROUND_BLUE : 0;
								break;
							default:
								break;
							}
							code = 0;
						}
					}
				}
			}
		}

		// don't call this while io is pending
		void flush() {
			assert(!m_pendingio);
			auto textfun = [this](const char *pbegin, size_t count) { handleText(pbegin, count); };
			auto escapefun = [this](const char *pbegin, size_t count) { handleEscape(pbegin, count); };
			m_pwrite = processEscapeSequences(&m_buf.front(), m_pwrite - &m_buf.front(), textfun, escapefun);
			FlushFileBuffers(m_hreal);
		}

		bool pending() const noexcept {
			return m_pendingio;
		}

		FILE * file() const noexcept {
			return m_fp;
		}

		~Redirection() {
			close();
		}
	};

	// active redirections
	std::vector<std::unique_ptr<Redirection>> redirections;

	// event to signal redirect worker should exit
	HANDLE redirect_should_exit = INVALID_HANDLE_VALUE;

	void doStdIORedirection() {
		try {
			// get event handles for redirections
			std::vector<HANDLE> events;
			for (const auto &rd : redirections) {
				events.push_back(rd->eventHandle());
			}

			// also wait on should-exit event as lowest priority
			events.push_back(redirect_should_exit);

			// note that event index must match redirection index

			//fprintf(stderr, "redirected!\n");

			// set to true when loop can exit cleanly
			bool done = false;

			// read and push data
			while (!done) {

				// wait for data
				int waitresult = WaitForMultipleObjects(events.size(), &events.front(), false, INFINITE);

				// get signaled event
				int i = waitresult - WAIT_OBJECT_0;
				if (i < 0 || i >= events.size()) {
					throwLastWin32Error("wait for pipe");
				}

				if (i < redirections.size()) {
					// event was for a redirection
					// push data through redirect
					// if last io was cancelled, this resets the event and does not start new io
					redirections[i]->onEventSignaled();
				}

				// was the exit event signaled?
				bool should_exit = false;
				switch (WaitForSingleObject(redirect_should_exit, 0)) {
				case WAIT_OBJECT_0:
					should_exit = true;
					break;
				case WAIT_TIMEOUT:
					break;
				default:
					throwLastWin32Error("test event");
				}

				// cancel any pending io if we need to exit
				if (should_exit) {
					for (const auto &rd : redirections) {
						rd->cancel();
					}
				}

				// if exit was signaled and all redirections have no pending io, we're done
				done = should_exit;
				for (const auto &rd : redirections) {
					done &= !rd->pending();
				}
			}

		} catch (gecom::win32_error &e) {
			// report any errors
			fprintf(stderr, "%s\n", e.what());
			return;
		}
	}

	class StdIORedirectInit {
	public:
		StdIORedirectInit() {
			//fprintf(stderr, "redirecting...\n");
			// create exit event for redirect worker
			redirect_should_exit = CreateEvent(nullptr, true, false, nullptr);
			if (redirect_should_exit == INVALID_HANDLE_VALUE) {
				fprintf(stderr, "%s\n", win32_error(GetLastError(), "create event").what());
				return;
			}
			// create redirections
			redirections.push_back(std::make_unique<Redirection>(stderr, 256));
			redirections.push_back(std::make_unique<Redirection>(stdout, 256));
			// start thread
			redirect_worker = std::thread(doStdIORedirection);
		}

		~StdIORedirectInit() {
			if (redirect_worker.joinable()) {
				//fprintf(stdout, "goodbye from redirected stdout\n");
				//fprintf(stderr, "goodbye from redirected stderr\n");
				//fflush(stdout);
				//fflush(stderr);
				// dup handles, freopen to conout$, flush pipes
				std::vector<HANDLE> hdups;
				for (const auto &rd : redirections) {
					HANDLE hpipew;
					// get pipe write handle duplicate
					// freopen causes the original to be closed
					DuplicateHandle(
						GetCurrentProcess(),
						HANDLE(_get_osfhandle(_fileno(rd->file()))),
						GetCurrentProcess(),
						&hpipew,
						0,
						false,
						DUPLICATE_SAME_ACCESS
					);
					hdups.push_back(hpipew);
					// freopen file to conout$
					freopen("CONOUT$", "w", rd->file());
					// flush
					FlushFileBuffers(hpipew);
				}
				// signal worker should exit
				SetEvent(redirect_should_exit);
				// wait for worker to stop
				redirect_worker.join();
				// close duplicate handles
				for (HANDLE h : hdups) {
					CloseHandle(h);
				}
				//fprintf(stderr, "redirection stopped\n");
			}
		}
	};

	StdIORedirectInit stdio_redirect_init_obj;

#elif defined(GECOM_STDIO_REDIRECT_POSIX)
#define GECOM_TERMINAL_ANSI

// TODO posix-land stdio redirect

#endif // GECOM_STDIO_REDIRECT_*

}

namespace gecom {

	namespace terminal {

#ifdef GECOM_TERMINAL_ANSI
		std::ostream & reset(std::ostream &o) { o << "\033[0m"; return o; }

		std::ostream & black(std::ostream &o) { o << "\033[0;30m"; return o; }
		std::ostream & red(std::ostream &o) { o << "\033[0;31m"; return o; }
		std::ostream & green(std::ostream &o) { o << "\033[0;32m"; return o; }
		std::ostream & yellow(std::ostream &o) { o << "\033[0;33m"; return o; }
		std::ostream & blue(std::ostream &o) { o << "\033[0;34m"; return o; }
		std::ostream & magenta(std::ostream &o) { o << "\033[0;35m"; return o; }
		std::ostream & cyan(std::ostream &o) { o << "\033[0;36m"; return o; }
		std::ostream & white(std::ostream &o) { o << "\033[0;37m"; return o; }

		std::ostream & boldBlack(std::ostream &o) { o << "\033[0;1;30m"; return o; }
		std::ostream & boldRed(std::ostream &o) { o << "\033[0;1;31m"; return o; }
		std::ostream & boldGreen(std::ostream &o) { o << "\033[0;1;32m"; return o; }
		std::ostream & boldYellow(std::ostream &o) { o << "\033[0;1;33m"; return o; }
		std::ostream & boldBlue(std::ostream &o) { o << "\033[0;1;34m"; return o; }
		std::ostream & boldMagenta(std::ostream &o) { o << "\033[0;1;35m"; return o; }
		std::ostream & boldCyan(std::ostream &o) { o << "\033[0;1;36m"; return o; }
		std::ostream & boldWhite(std::ostream &o) { o << "\033[0;1;37m"; return o; }

		std::ostream & onBlack(std::ostream &o) { o << "\033[40m"; return o; }
		std::ostream & onRed(std::ostream &o) { o << "\033[41m"; return o; }
		std::ostream & onGreen(std::ostream &o) { o << "\033[42m"; return o; }
		std::ostream & onYellow(std::ostream &o) { o << "\033[43m"; return o; }
		std::ostream & onBlue(std::ostream &o) { o << "\033[44m"; return o; }
		std::ostream & onMagenta(std::ostream &o) { o << "\033[45m"; return o; }
		std::ostream & onCyan(std::ostream &o) { o << "\033[46m"; return o; }
		std::ostream & onWhite(std::ostream &o) { o << "\033[47m"; return o; }
#else
		std::ostream & reset(std::ostream &o) { return o; }

		std::ostream & black(std::ostream &o) { return o; }
		std::ostream & red(std::ostream &o) { return o; }
		std::ostream & green(std::ostream &o) { return o; }
		std::ostream & yellow(std::ostream &o) { return o; }
		std::ostream & blue(std::ostream &o) { return o; }
		std::ostream & magenta(std::ostream &o) { return o; }
		std::ostream & cyan(std::ostream &o) { return o; }
		std::ostream & white(std::ostream &o) { return o; }

		std::ostream & boldBlack(std::ostream &o) { return o; }
		std::ostream & boldRed(std::ostream &o) { return o; }
		std::ostream & boldGreen(std::ostream &o) { return o; }
		std::ostream & boldYellow(std::ostream &o) { return o; }
		std::ostream & boldBlue(std::ostream &o) { return o; }
		std::ostream & boldMagenta(std::ostream &o) { return o; }
		std::ostream & boldCyan(std::ostream &o) { return o; }
		std::ostream & boldWhite(std::ostream &o) { return o; }

		std::ostream & onBlack(std::ostream &o) { return o; }
		std::ostream & onRed(std::ostream &o) { return o; }
		std::ostream & onGreen(std::ostream &o) { return o; }
		std::ostream & onYellow(std::ostream &o) { return o; }
		std::ostream & onBlue(std::ostream &o) { return o; }
		std::ostream & onMagenta(std::ostream &o) { return o; }
		std::ostream & onCyan(std::ostream &o) { return o; }
		std::ostream & onWhite(std::ostream &o) { return o; }
#endif

	}
}
