#ifndef GECOM_LOG_HPP
#define GECOM_LOG_HPP

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "Section.hpp"

namespace gecom {

	namespace termcolor {

		// Terminal Color Manipulators (use these like std::endl on std::cout/cerr/clog)

		// Reset Color
		std::ostream & reset(std::ostream &);
		
		// Regular Colors
		std::ostream & black(std::ostream &);
		std::ostream & red(std::ostream &);
		std::ostream & green(std::ostream &);
		std::ostream & yellow(std::ostream &);
		std::ostream & blue(std::ostream &);
		std::ostream & purple(std::ostream &);
		std::ostream & cyan(std::ostream &);
		std::ostream & white(std::ostream &);
		
		// Bold Colors
		std::ostream & boldBlack(std::ostream &);
		std::ostream & boldRed(std::ostream &);
		std::ostream & boldGreen(std::ostream &);
		std::ostream & boldYellow(std::ostream &);
		std::ostream & boldBlue(std::ostream &);
		std::ostream & boldPurple(std::ostream &);
		std::ostream & boldCyan(std::ostream &);
		std::ostream & boldWhite(std::ostream &);

	}

	enum class loglevel {
		info, warning, error, critical
	};

	template <typename CharT>
	class basic_logstream;
	
	using logstream = basic_logstream<char>;

	class LogOutput {
	private:
		unsigned m_verbosity = 9001;
		bool m_mute = false;

	protected:
		// this is responsible for trailing newlines
		virtual void write_impl(unsigned verbosity, loglevel level, const std::string &hdr, const std::string &msg) = 0;

	public:
		LogOutput(const LogOutput &) = delete;
		LogOutput & operator=(const LogOutput &) = delete;

		LogOutput(bool mute_ = false) : m_mute(mute_) { }

		unsigned verbosity() {
			return m_verbosity;
		}

		void verbosity(unsigned v) {
			m_verbosity = v;
		}

		bool mute() {
			return m_mute;
		}

		void mute(bool b) {
			m_mute = b;
		}

		void write(unsigned verbosity, loglevel level, const std::string &hdr, const std::string &msg) {
			if (!m_mute && verbosity < m_verbosity) write_impl(verbosity, level, hdr, msg);
		}

		virtual ~LogOutput() { }

	};

	// log output that writes to a std::ostream
	class StreamLogOutput : public LogOutput {
	private:
		std::ostream *m_out;

	protected:
		virtual void write_impl(unsigned, loglevel, const std::string &hdr, const std::string &msg) override {
			(*m_out) << hdr << msg << std::endl;
		}

		std::ostream & stream() {
			return *m_out;
		}

	public:
		explicit StreamLogOutput(std::ostream *out_, bool mute_ = false) : LogOutput(mute_), m_out(out_) { }
	};

	// log output for writing to std::cout/cerr/clog (as they are the only streams with reliable color support)
	class ColoredStreamLogOutput : public StreamLogOutput {
	protected:
		virtual void write_impl(unsigned verbosity, loglevel level, const std::string &hdr, const std::string &msg) override {
			std::ostream &out = stream();
			using namespace std;
			if (verbosity < 2) {
				switch (level) {
				case loglevel::warning:
					out << termcolor::boldYellow; break;
				case loglevel::error:
					out << termcolor::boldRed; break;
				default:
					out << termcolor::boldGreen;
				}
			} else {
				switch (level) {
				case loglevel::warning:
					out << termcolor::yellow; break;
				case loglevel::error:
					out << termcolor::red; break;
				default:
					out << termcolor::green;
				}
			}
			out << hdr;
			out << termcolor::reset;
			out << msg;
			out << endl;
		}

	public:
		explicit ColoredStreamLogOutput(std::ostream *out_, bool mute_ = false) : StreamLogOutput(out_, mute_) { }
	};

	class FileLogOutput : public StreamLogOutput {
	private:
		std::ofstream m_out;

	public:
		explicit FileLogOutput(
			const std::string &fname_,
			std::ios_base::openmode mode_ = std::ios::trunc,
			bool mute_ = false
		) :
			StreamLogOutput(&m_out, mute_)
		{
			m_out.open(fname_, mode_);
			if (!m_out.is_open()) {
				throw std::runtime_error("unable to open file");
			}
		}
	};

	class Log {
	private:
		Log() = delete;
		Log(const Log &) = delete;
		Log & operator=(const Log &) = delete;

		static std::mutex m_mutex;
		static std::unordered_set<LogOutput *> m_outputs;

		// stdout starts muted, stderr starts non-muted
		static ColoredStreamLogOutput m_stdout;
		static ColoredStreamLogOutput m_stderr;

	public:
		static constexpr unsigned defaultVerbosity(loglevel l) {
			return l == loglevel::critical ? 0 : (l == loglevel::error ? 1 : (l == loglevel::warning ? 2 : 3));
		}
		
		static void write(unsigned verbosity, loglevel level, const std::string &source, const std::string &msg);

		static void addOutput(LogOutput *out) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_outputs.insert(out);
		}

		static void removeOutput(LogOutput *out) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_outputs.erase(out);
		}

		static LogOutput & stdOut() {
			return m_stdout;
		}

		static LogOutput & stdErr() {
			return m_stderr;
		}

		static logstream info(const std::string &source = "");

		static logstream warning(const std::string &source = "");

		static logstream error(const std::string &source = "");

		static logstream critical(const std::string &source = "");

	};
	
	template <typename CharT>
	class basic_logstream : public std::basic_ostream<CharT> {
	private:
		unsigned m_verbosity;
		loglevel m_level;
		std::string m_source;
		std::basic_stringbuf<CharT> m_buf;
		bool m_write;

	public:
		basic_logstream(const basic_logstream &) = delete;
		basic_logstream & operator=(const basic_logstream &) = delete;

		// ctor; sets level to loglevel::info and verbosity to 3
		basic_logstream(std::string source_) :
			std::basic_ostream<CharT>(&m_buf),
			m_verbosity(3),
			m_level(loglevel::info),
			m_source(std::move(source_)),
			m_write(true)
		{ }

		// move ctor; takes over responsibility for writing to log
		basic_logstream(basic_logstream &&other) :
			std::basic_ostream<CharT>(&m_buf),
			m_verbosity(other.m_verbosity),
			m_level(other.m_level),
			m_source(std::move(other.m_source)),
			m_write(other.m_write)
		{
			other.m_write = false;
			using std::swap;
			swap(m_buf, other.m_buf);
		}

		// set level to loglevel::info and verbosity to default by default
		basic_logstream & info(unsigned v = Log::defaultVerbosity(loglevel::info)) {
			m_level = loglevel::info;
			m_verbosity = v;
			return *this;
		}

		// set level to loglevel::warning and verbosity to default by default
		basic_logstream & warning(unsigned v = Log::defaultVerbosity(loglevel::warning)) {
			m_level = loglevel::warning;
			m_verbosity = v;
			return *this;
		}

		// set level to loglevel::error and verbosity to default by default
		basic_logstream & error(unsigned v = Log::defaultVerbosity(loglevel::error)) {
			m_level = loglevel::error;
			m_verbosity = v;
			return *this;
		}

		// set level to loglevel::critical and verbosity to default by default
		basic_logstream & critical(unsigned v = Log::defaultVerbosity(loglevel::critical)) {
			m_level = loglevel::critical;
			m_verbosity = v;
			return *this;
		}

		// set verbosity
		basic_logstream & verbosity(unsigned v) {
			m_verbosity = v;
			return *this;
		}

		~basic_logstream() {
			if (m_write) {
				Log::write(m_verbosity, m_level, m_source, m_buf.str());
			}
		}

	};

	// TODO remove this
	inline logstream log(const std::string &source = "Global") {
		return Log::info(source);
	}

}

#endif // GECOM_LOG_HPP
