
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Are we posix?
#ifdef __posix
#include <unistd.h>
#include <time.h>
// Posix defines gmtime_r(), which is threadsafe
#define GECOM_HAVE_GMTIME_R
// Posix defines getpid()
#define GECOM_HAVE_GETPID
#endif

// is our gmtime() threadsafe?
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// MSVC stdlib uses TLS; is a DLL so must be threadsafe
// MinGW implements gmtime_r() as a macro around gmtime(), so gmtime() must be threadsafe
#define GECOM_GMTIME_THREADSAFE
#endif

#if !(defined(GECOM_GMTIME_THREADSAFE) || defined(GECOM_HAVE_GMTIME_R))
#error unable to find threadsafe gmtime() or alternative
#endif

#include <ctime>
#include <thread>

#include "Section.hpp"
#include "Log.hpp"

namespace gecom {

	namespace termcolor {

#ifdef GECOM_NO_TERMCOLOR
		// Reset Color
		std::ostream & reset(std::ostream &o) { return o; }

		// Regular Colors
		std::ostream & black(std::ostream &o) { return o; }
		std::ostream & red(std::ostream &o) { return o; }
		std::ostream & green(std::ostream &o) { return o; }
		std::ostream & yellow(std::ostream &o) { return o; }
		std::ostream & blue(std::ostream &o) { return o; }
		std::ostream & purple(std::ostream &o) { return o; }
		std::ostream & cyan(std::ostream &o) { return o; }
		std::ostream & white(std::ostream &o) { return o; }

		// Bold Colors
		std::ostream & boldBlack(std::ostream &o) { return o; }
		std::ostream & boldRed(std::ostream &o) { return o; }
		std::ostream & boldGreen(std::ostream &o) { return o; }
		std::ostream & boldYellow(std::ostream &o) { return o; }
		std::ostream & boldBlue(std::ostream &o) { return o; }
		std::ostream & boldPurple(std::ostream &o) { return o; }
		std::ostream & boldCyan(std::ostream &o) { return o; }
		std::ostream & boldWhite(std::ostream &o) { return o; }

#elif defined(_WIN32)
		// use windows console manip

		namespace {
			HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
			HANDLE hstderr = GetStdHandle(STD_ERROR_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO *csbi_out = nullptr;
			CONSOLE_SCREEN_BUFFER_INFO *csbi_err = nullptr;

			void checkConsoleDefaultsSaved() {
				// save the original values for reset
				if (!csbi_out) {
					csbi_out = new CONSOLE_SCREEN_BUFFER_INFO;
					GetConsoleScreenBufferInfo(hstdout, csbi_out);
				}
				if (!csbi_err) {
					csbi_err = new CONSOLE_SCREEN_BUFFER_INFO;
					GetConsoleScreenBufferInfo(hstderr, csbi_err);
				}
			}

			void resetTextAttribute(std::ostream &o) {
				checkConsoleDefaultsSaved();
				if (&o == &std::cout) {
					SetConsoleTextAttribute(hstdout, csbi_out->wAttributes);
				}
				if (&o == &std::cerr || &o == &std::clog) {
					SetConsoleTextAttribute(hstderr, csbi_err->wAttributes);
				}
			}

			void setTextColor(std::ostream &o, WORD w) {
				checkConsoleDefaultsSaved();
				// set color, preserving current background
				// apparently i dont need to explicitly flush before setting the color
				if (&o == &std::cout) {
					w = w & 0x0F;
					CONSOLE_SCREEN_BUFFER_INFO csbi;
					GetConsoleScreenBufferInfo(hstdout, &csbi);
					w = (csbi.wAttributes & 0xF0) | w;
					SetConsoleTextAttribute(hstdout, w);
				}
				if (&o == &std::cerr || &o == &std::clog) {
					w = w & 0x0F;
					CONSOLE_SCREEN_BUFFER_INFO csbi;
					GetConsoleScreenBufferInfo(hstderr, &csbi);
					w = (csbi.wAttributes & 0xF0) | w;
					SetConsoleTextAttribute(hstderr, w);
				}
			}

		}

		// Reset Color
		std::ostream & reset(std::ostream &o) { resetTextAttribute(o); return o; }

		// Regular Colors
		std::ostream & black(std::ostream &o) { setTextColor(o, 0); return o; }
		std::ostream & red(std::ostream &o) { setTextColor(o, 4); return o; }
		std::ostream & green(std::ostream &o) { setTextColor(o, 2); return o; }
		std::ostream & yellow(std::ostream &o) { setTextColor(o, 6); return o; }
		std::ostream & blue(std::ostream &o) { setTextColor(o, 1); return o; }
		std::ostream & purple(std::ostream &o) { setTextColor(o, 5); return o; }
		std::ostream & cyan(std::ostream &o) { setTextColor(o, 3); return o; }
		std::ostream & white(std::ostream &o) { setTextColor(o, 7); return o; }

		// Bold Colors
		std::ostream & boldBlack(std::ostream &o) { setTextColor(o, 8); return o; }
		std::ostream & boldRed(std::ostream &o) { setTextColor(o, 12); return o; }
		std::ostream & boldGreen(std::ostream &o) { setTextColor(o, 10); return o; }
		std::ostream & boldYellow(std::ostream &o) { setTextColor(o, 14); return o; }
		std::ostream & boldBlue(std::ostream &o) { setTextColor(o, 9); return o; }
		std::ostream & boldPurple(std::ostream &o) { setTextColor(o, 13); return o; }
		std::ostream & boldCyan(std::ostream &o) { setTextColor(o, 11); return o; }
		std::ostream & boldWhite(std::ostream &o) { setTextColor(o, 15); return o; }

#else
		// use unix escape codes
		
		namespace {

			void setTextAttribute(std::ostream &o, const char *attrib) {
				if (&o == &std::cout || &o == &std::cerr || &o == &std::clog) {
					// only allow escape codes going to stdout/stderr
					o << attrib;
				}
			}

		}

		// Reset Color
		std::ostream & reset(std::ostream &o) { setTextAttribute(o, "\033[0m"); return o; }

		// Regular Colors
		std::ostream & black(std::ostream &o) { setTextAttribute(o, "\033[0;30m"); return o; }
		std::ostream & red(std::ostream &o) { setTextAttribute(o, "\033[0;31m"); return o; }
		std::ostream & green(std::ostream &o) { setTextAttribute(o, "\033[0;32m"); return o; }
		std::ostream & yellow(std::ostream &o) { setTextAttribute(o, "\033[0;33m"); return o; }
		std::ostream & blue(std::ostream &o) { setTextAttribute(o, "\033[0;34m"); return o; }
		std::ostream & purple(std::ostream &o) { setTextAttribute(o, "\033[0;35m"); return o; }
		std::ostream & cyan(std::ostream &o) { setTextAttribute(o, "\033[0;36m"); return o; }
		std::ostream & white(std::ostream &o) { setTextAttribute(o, "\033[0;37m"); return o; }

		// Bold Colors
		std::ostream & boldBlack(std::ostream &o) { setTextAttribute(o, "\033[1;30m"); return o; }
		std::ostream & boldRed(std::ostream &o) { setTextAttribute(o, "\033[1;31m"); return o; }
		std::ostream & boldGreen(std::ostream &o) { setTextAttribute(o, "\033[1;32m"); return o; }
		std::ostream & boldYellow(std::ostream &o) { setTextAttribute(o, "\033[1;33m"); return o; }
		std::ostream & boldBlue(std::ostream &o) { setTextAttribute(o, "\033[1;34m"); return o; }
		std::ostream & boldPurple(std::ostream &o) { setTextAttribute(o, "\033[1;35m"); return o; }
		std::ostream & boldCyan(std::ostream &o) { setTextAttribute(o, "\033[1;36m"); return o; }
		std::ostream & boldWhite(std::ostream &o) { setTextAttribute(o, "\033[1;37m"); return o; }
#endif

	}

	namespace {
		auto processID() {
#ifdef _WIN32
			return GetCurrentProcessId();
#elif defined(GECOM_HAVE_GETPID)
			return uintmax_t(getpid());
#else
			return "???";
#endif
		}
	}

	std::unordered_set<LogOutput *> Log::m_outputs;
	std::mutex Log::m_mutex;

	ConsoleLogOutput Log::m_stdout(&std::cout, true);
	ConsoleLogOutput Log::m_stderr(&std::clog, false);

	void Log::write(unsigned verbosity, loglevel level, const std::string &source, const std::string &body) {
		std::unique_lock<std::mutex> lock(m_mutex);

		using namespace std::chrono;

		// truncate to seconds, use difference for second-fraction part of timestamp
		auto t1 = system_clock::now();
		auto t0 = time_point_cast<seconds>(t1);
		std::time_t tt = std::chrono::system_clock::to_time_t(t0);
		
		// who the fuck thought tm was a good struct name?
		std::tm *t = nullptr;

		// why is this shit so terrible? WHY?
#ifdef GECOM_HAVE_GMTIME_R
		std::tm bullshit;
		t = &bullshit;
		gmtime_r(&tt, t);
#else
		t = std::gmtime(&tt);
#endif
		
		// format time: https://www.ietf.org/rfc/rfc3339.txt
		// 2015-07-29T12:43:15.123Z
		// we always use 3 digit second-fraction
		std::ostringstream timess;
		timess << std::setfill('0');
		timess << std::setw(4) << (1900 + t->tm_year) << '-';
		timess << std::setw(2) << (1 + t->tm_mon) << '-';
		timess << std::setw(2) << t->tm_mday << 'T';
		timess << std::setw(2) << t->tm_hour << ':';
		timess << std::setw(2) << t->tm_min << ':';
		timess << std::setw(2) << t->tm_sec << '.';
		timess << std::setw(3) << duration_cast<milliseconds>(t1 - t0).count() << 'Z';

		// prepare the message
		logmessage msg;
		msg.time = timess.str();
		msg.level = level;
		msg.verbosity = verbosity;
		msg.source = source;
		msg.body = body;

		// write to stderr and stdout
		m_stderr.write(msg);
		m_stdout.write(msg);

		// write to all others
		for (LogOutput *out : m_outputs) {
			out->write(msg);
		}
		
	}

	logstream Log::info(const std::string &source) {
		std::ostringstream fullsource;
		fullsource << processID() << '/';
		fullsource << std::this_thread::get_id() << '/';
		if (const Section *sec = Section::current()) {
			fullsource << sec->path();
		}
		if (!source.empty()) {
			fullsource << '/';
			fullsource << source;
		}
		return logstream(fullsource.str());
	}

	logstream Log::warning(const std::string &source) {
		return std::move(info().warning());
	}

	logstream Log::error(const std::string &source) {
		return std::move(info().error());
	}

	logstream Log::critical(const std::string &source) {
		return std::move(info().critical());
	}

}
