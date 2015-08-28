
#include "Platform.hpp"

#ifdef GECOM_PLATFORM_WIN32
#include <windows.h>
#endif

// Are we posix-ish?
#ifdef GECOM_PLATFORM_POSIX
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
// Posix defines gmtime_r(), which is threadsafe
#define GECOM_HAVE_GMTIME_R
// Posix defines getpid()
#define GECOM_HAVE_GETPID
// We probably have ioctl() for terminal size
#define GECOM_HAVE_IOCTL
#endif

// is our gmtime() threadsafe?
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// MSVC stdlib uses TLS
// MinGW implements gmtime_r() as a macro around gmtime(), so gmtime() must be threadsafe
#define GECOM_GMTIME_THREADSAFE
#endif

#if !(defined(GECOM_GMTIME_THREADSAFE) || defined(GECOM_HAVE_GMTIME_R))
#error unable to find threadsafe gmtime() or alternative
#endif

#include <cctype>
#include <ctime>
#include <thread>
#include <sstream>

#include "Log.hpp"
#include "Terminal.hpp"

namespace {
	auto processID() {
#ifdef _WIN32
		return GetCurrentProcessId();
#elif defined(GECOM_HAVE_GETPID)
		return getpid();
#else
		return "???";
#endif
	}

	int consoleWidth() {
#ifdef _WIN32
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(h, &csbi);
		return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#elif defined(GECOM_HAVE_IOCTL)
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		return w.ws_col;
#else
		return 80;
#endif
	}

	// log output for writing to console via std::cout/cerr/clog (with color)
	class ConsoleLogOutput : public gecom::StreamLogOutput {
	protected:
		virtual void writeImpl(const gecom::logmessage &msg) override;

	public:
		explicit ConsoleLogOutput(std::ostream *out_, bool mute_ = false) : StreamLogOutput(out_, mute_) { }
	};

	void ConsoleLogOutput::writeImpl(const gecom::logmessage &msg) {
		namespace terminal = gecom::terminal;
		using gecom::loglevel;

		std::ostream &out = *stream();

		// colorization
		std::ostream & (*levelcolor)(std::ostream &);
		std::ostream & (*delimcolor)(std::ostream &);

		if (msg.verbosity < 2) {
			delimcolor = terminal::boldCyan;
			switch (msg.level) {
			case loglevel::warning:
				levelcolor = terminal::boldYellow;
				break;
			case loglevel::error:
			case loglevel::critical:
				levelcolor = terminal::boldRed;
				break;
			default:
				levelcolor = terminal::boldGreen;
				break;
			}
		} else {
			delimcolor = terminal::cyan;
			switch (msg.level) {
			case loglevel::warning:
				levelcolor = terminal::yellow;
				break;
			case loglevel::error:
			case loglevel::critical:
				levelcolor = terminal::red;
				break;
			default:
				levelcolor = terminal::green;
				break;
			}
		}

		// date and time
		for (auto c : msg.time) {
			if (std::isdigit(c)) {
				out << terminal::cyan << c;
			} else if (std::isalpha(c)) {
				out << terminal::blue << c;
			} else {
				out << terminal::boldBlack << c;
			}
		}
		out << delimcolor << " | ";

		// verbosity and level
		out << levelcolor << msg.verbosity << delimcolor << "> " << levelcolor << std::setw(11) << msg.level;

		// source
		out << delimcolor << " [" << terminal::reset;
		out << levelcolor << ' ' << msg.source << ' ';
		out << delimcolor << "]" << terminal::reset;

		// do we need to start body on new line?
		// TODO check msg size against console width and cursor col
		if (msg.body.find_first_of("\r\n") != std::string::npos || msg.body.size() > 50) {
			out << '\n';
		} else {
			out << delimcolor << " : " << terminal::reset;
		}

		// message body
		out << msg.body;
		out << std::endl;
	}

	class DebugLogOutput : public gecom::LogOutput {
	protected:
		virtual void writeImpl(const gecom::logmessage &msg) override;

	public:
		explicit DebugLogOutput(bool mute_ = false) : LogOutput(mute_) { }
	};

#ifdef GECOM_PLATFORM_WIN32
	void DebugLogOutput::writeImpl(const gecom::logmessage &msg) {
		std::ostringstream oss;
		oss << msg << std::endl;
		OutputDebugStringA(oss.str().c_str());
	}
#else
	void DebugLogOutput::writeImpl(const gecom::logmessage &msg) { }
#endif

	// stdio log outputs
	ConsoleLogOutput stdout_logoutput { &std::cout, true };
	ConsoleLogOutput stderr_logoutput { &std::clog, false };

	// debug log output
	DebugLogOutput debug_logoutput { false };
}

namespace gecom {

	std::unordered_set<LogOutput *> Log::m_outputs;
	std::mutex Log::m_mutex;

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

		if (t) {
			timess << std::setfill('0');
			timess << std::setw(4) << (1900 + t->tm_year) << '-';
			timess << std::setw(2) << (1 + t->tm_mon) << '-';
			timess << std::setw(2) << t->tm_mday << 'T';
			timess << std::setw(2) << t->tm_hour << ':';
			timess << std::setw(2) << t->tm_min << ':';
			timess << std::setw(2) << t->tm_sec << '.';
			timess << std::setw(3) << duration_cast<milliseconds>(t1 - t0).count() << 'Z';
		} else {
			timess << "0000-00-00T00:00:00.000Z";
		}

		// prepare the message
		logmessage msg;
		msg.time = timess.str();
		msg.level = level;
		msg.verbosity = verbosity;
		msg.source = source;
		msg.body = body;

		// write to debug
		debug_logoutput.write(msg);

		// write to stdio
		stderr_logoutput.write(msg);
		stdout_logoutput.write(msg);

		// write to all others
		for (LogOutput *out : m_outputs) {
			out->write(msg);
		}
		
	}

	LogOutput * Log::stdOut() {
		return &stdout_logoutput;
	}

	LogOutput * Log::stdErr() {
		return &stderr_logoutput;
	}

	LogOutput * Log::debugOut() {
		return &debug_logoutput;
	}

	logstream Log::info(const std::string &source) {
		std::ostringstream fullsource;
		fullsource << processID() << '/';
		fullsource << std::this_thread::get_id() << '/';
		if (const section *sec = section::current()) {
			fullsource << sec->path();
		}
		if (!source.empty()) {
			fullsource << '/';
			fullsource << source;
		}
		return logstream(fullsource.str());
	}

	logstream Log::warning(const std::string &source) {
		return std::move(info(source).warning());
	}

	logstream Log::error(const std::string &source) {
		return std::move(info(source).error());
	}

	logstream Log::critical(const std::string &source) {
		return std::move(info(source).critical());
	}

}
