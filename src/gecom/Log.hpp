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

	enum class loglevel {
		info, warning, error, critical
	};

	inline std::ostream & operator<<(std::ostream &out, loglevel l) {
		switch (l) {
		case loglevel::info:
			out << "Information";
			break;
		case loglevel::warning:
			out << "Warning";
			break;
		case loglevel::error:
			out << "Error";
			break;
		case loglevel::critical:
			out << "Critical";
			break;
		default:
			out << "???";
			break;
		}
		return out;
	}

	template <typename CharT>
	class basic_logstream;
	
	using logstream = basic_logstream<char>;

	struct logmessage {
		// RFC3339 time string https://www.ietf.org/rfc/rfc3339.txt
		std::string time;
		loglevel level;
		unsigned verbosity;
		std::string source;
		std::string body;

		inline friend std::ostream & operator<<(std::ostream &out, const logmessage &m) {
			// 2015-07-28T02:20:42.123Z | 0>    Error [ 1/1/main/gameloop/draw/terrain/GL:API ] : Invalid Operation
			out << m.time << " | " << m.verbosity << "> " << std::setw(11) << m.level << " [";
			out << m.source << "] : ";
			if (m.body.find_first_of("\r\n") != std::string::npos) {
				// start multiline messages on a new line
				out << '\n';
			}
			out << m.body;
			return out;
		}
	};

	class LogOutput {
	private:
		unsigned m_verbosity = 9001;
		bool m_mute = false;

	protected:
		// this is responsible for trailing newlines
		virtual void writeImpl(const logmessage &msg) = 0;

	public:
		LogOutput(const LogOutput &) = delete;
		LogOutput & operator=(const LogOutput &) = delete;

		LogOutput(bool mute_ = false) : m_mute(mute_) { }

		unsigned verbosity() const {
			return m_verbosity;
		}

		void verbosity(unsigned v) {
			m_verbosity = v;
		}

		bool mute() const {
			return m_mute;
		}

		void mute(bool b) {
			m_mute = b;
		}

		void write(const logmessage &msg) {
			if (!m_mute && msg.verbosity < m_verbosity) writeImpl(msg);
		}

		virtual ~LogOutput() { }

	};

	// log output that writes to a std::ostream
	class StreamLogOutput : public LogOutput {
	private:
		std::ostream *m_out;

	protected:
		virtual void writeImpl(const logmessage &msg) override {
			(*m_out) << msg << std::endl;
		}

		std::ostream * stream() const {
			return m_out;
		}

	public:
		explicit StreamLogOutput(std::ostream *out_, bool mute_ = false) : LogOutput(mute_), m_out(out_) { }
	};

	// log output that writes to a file
	class FileLogOutput : public StreamLogOutput {
	private:
		std::ofstream m_out;

	public:
		explicit FileLogOutput(
			const std::string &fname_,
			std::ios::openmode mode_ = std::ios::trunc,
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

	public:
		static constexpr unsigned defaultVerbosity(loglevel l) {
			return l == loglevel::critical ? 0 : (l == loglevel::error ? 1 : (l == loglevel::warning ? 2 : 3));
		}
		
		static void write(unsigned verbosity, loglevel level, const std::string &source, const std::string &body);

		static void addOutput(LogOutput *out) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_outputs.insert(out);
		}

		static void removeOutput(LogOutput *out) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_outputs.erase(out);
		}

		// stdout starts muted
		static LogOutput * stdOut();

		// stderr starts unmuted
		static LogOutput * stdErr();

		// system debug output, if available; starts unmuted
		static LogOutput * debugOut();

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

		// set verbosity as default verbosity for a loglevel
		basic_logstream & verbosity(loglevel l) {
			m_verbosity = Log::defaultVerbosity(l);
			return *this;
		}

		~basic_logstream() {
			if (m_write) {
				Log::write(m_verbosity, m_level, m_source, m_buf.str());
			}
		}

	};

}

#endif // GECOM_LOG_HPP
