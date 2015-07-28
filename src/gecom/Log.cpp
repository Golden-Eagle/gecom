
#include "Section.hpp"
#include "Log.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

	std::unordered_set<LogOutput *> Log::m_outputs;
	std::mutex Log::m_mutex;

	ColoredStreamLogOutput Log::m_stdout(&std::cout, true);
	ColoredStreamLogOutput Log::m_stderr(&std::cerr, false);

	void Log::write(unsigned verbosity, loglevel level, const std::string &source, const std::string &msg) {
		// better format maybe?
		// Wed May 30 12:25:03 2012 [System] Error : IT BROEK

		// TODO:
		// 2015-07-28 02:20:42.123 | 0>    Error [mainthread/gameloop/draw/terrain/GL:API] : Invalid Operation

		std::unique_lock<std::mutex> lock(m_mutex);

		static const char *levelstr;
		switch (level) {
		case loglevel::info:
			levelstr = "Information";
			break;
		case loglevel::warning:
			levelstr = "Warning";
			break;
		case loglevel::error:
			levelstr = "Error";
			break;
		case loglevel::critical:
			levelstr = "Critical";
			break;
		default:
			levelstr = "???";
			break;
		}

		std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		std::string ts = std::string(ctime(&tt));
		ts.pop_back();

		// prepare the header
		std::ostringstream ss;
		ss << ts;
		ss << " [" << std::setw(15) << source << "] ";
		ss << std::setw(11) << levelstr << " : ";

		if (msg.find_first_of("\r\n") != std::string::npos || msg.length() > 50) {
			// message contains a CR or LF or is longer than 50 characters, start on a new line
			// TODO -> log output?
			ss << std::endl;
		}

		// write to stderr and stdout
		m_stderr.write(verbosity, level, ss.str(), msg);
		m_stdout.write(verbosity, level, ss.str(), msg);

		// write to all others
		for (LogOutput *out : m_outputs) {
			out->write(verbosity, level, ss.str(), msg);
		}
		
	}

	logstream Log::info(const std::string &source) {
		std::string fullsource = "???";
		if (const Section *sec = Section::current()) {
			fullsource = sec->path();
		}
		if (!source.empty()) {
			fullsource += '/';
			fullsource += source;
		}
		return logstream(fullsource);
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
