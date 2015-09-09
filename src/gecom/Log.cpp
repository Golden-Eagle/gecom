
#include "Platform.hpp"

#ifdef GECOM_PLATFORM_WIN32
#include <windows.h>
#endif

// Are we posix-ish?
#ifdef GECOM_PLATFORM_POSIX
#include <unistd.h>
#include <time.h>
// Posix defines gmtime_r(), which is threadsafe
#define GECOM_HAVE_GMTIME_R
// Posix defines getpid()
#define GECOM_HAVE_GETPID
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
#include <cstdio>
#include <climits>
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

	// log output for writing to console (with color)
	class ConsoleLogOutput : public gecom::LogOutput {
	protected:
		// we use FILE * in order to query the console width
		FILE *m_fp;

		virtual void writeImpl(const gecom::logmessage &msg) override;

	public:
		explicit ConsoleLogOutput(FILE *fp_, bool mute_ = false) : LogOutput(mute_), m_fp(fp_) {
			verbosity(4);
		}
	};

	void ConsoleLogOutput::writeImpl(const gecom::logmessage &msg) {
		namespace terminal = gecom::terminal;
		using gecom::loglevel;

		// dump to string first, then write to file
		std::ostringstream out;

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

		// verbosity and level
		out << delimcolor << " | ";
		out << levelcolor << msg.verbosity << delimcolor << "> " << levelcolor << std::setw(11) << msg.level;

		// source
		out << delimcolor << " [" << terminal::reset;
		out << levelcolor << ' ' << msg.source << ' ';
		out << delimcolor << "] : " << terminal::reset;

		// do we need to start body on new line?
		size_t totalwidth = 49 + msg.source.size() + msg.body.size();
		if (msg.body.find_first_of("\r\n") != std::string::npos || totalwidth >= terminal::width(m_fp)) {
			out << '\n';
		}

		// message body
		out << msg.body;
		out << std::endl;

		// write to output
		std::string outtext = out.str();
		fwrite(outtext.c_str(), 1, outtext.size(), m_fp);
		fflush(m_fp);
	}

	class DebugLogOutput : public gecom::LogOutput {
	protected:
		virtual void writeImpl(const gecom::logmessage &msg) override;

	public:
		explicit DebugLogOutput(bool mute_ = false) : LogOutput(mute_) {
			verbosity(4);
		}
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

	struct LogStatics {
		// stdio log outputs
		ConsoleLogOutput stdout_logoutput { stdout, true };
		ConsoleLogOutput stderr_logoutput { stderr, false };

		// debug log output
		DebugLogOutput debug_logoutput { false };

		// other log outputs
		std::mutex output_mutex;
		std::unordered_set<gecom::LogOutput *> outputs;

		~LogStatics() {
			gecom::Log::info() << "Log deinitialized";
		}
	};

	auto & logStatics() {
		static LogStatics s;
		return s;
	}
}

namespace gecom {

	void Log::write(unsigned verbosity, loglevel level, const std::string &source, const std::string &body) {
		std::unique_lock<std::mutex> lock(logStatics().output_mutex);

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
		logStatics().debug_logoutput.write(msg);

		// write to stdio
		logStatics().stderr_logoutput.write(msg);
		logStatics().stdout_logoutput.write(msg);

		// write to all others
		for (LogOutput *out : logStatics().outputs) {
			out->write(msg);
		}
		
	}

	void Log::addOutput(LogOutput *out) {
		std::unique_lock<std::mutex> lock(logStatics().output_mutex);
		logStatics().outputs.insert(out);
	}

	void Log::removeOutput(LogOutput *out) {
		std::unique_lock<std::mutex> lock(logStatics().output_mutex);
		logStatics().outputs.erase(out);
	}

	LogOutput * Log::stdOut() {
		return &logStatics().stdout_logoutput;
	}

	LogOutput * Log::stdErr() {
		return &logStatics().stderr_logoutput;
	}

	LogOutput * Log::debugOut() {
		return &logStatics().debug_logoutput;
	}

	logstream Log::info(const std::string &source) {
		std::ostringstream fullsource;
		fullsource << processID() << '/';
		fullsource << std::this_thread::get_id() << '/';
		if (!source.empty()) {
			section_guard sec(source);
			fullsource << section::current()->path();
		} else if (const section *sec = section::current()) {
			fullsource << sec->path();
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

	size_t LogInit::refcount = 0;

	LogInit::LogInit() {
		if (refcount++ == 0) {
			// ensure log statics are initialized
			logStatics();
			Log::info() << "Log initialized";
		}
	}

	LogInit::~LogInit() {
		if (--refcount == 0) {
			// log statics about to be destroyed
			// nothing to do
		}
	}
}
