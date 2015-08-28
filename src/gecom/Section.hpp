
#ifndef GECOM_SECTION_HPP
#define GECOM_SECTION_HPP

#include <utility>
#include <string>
#include <chrono>

namespace gecom {

	class section_guard {
	private:
		bool m_entered = false;
		std::string m_name;

	public:
		section_guard() { }
		
		section_guard(std::string name_);

		~section_guard();
	};

	class section {
		friend class section_guard;
	public:
		using clock = std::chrono::high_resolution_clock;
		using time_point = clock::time_point;

	private:
		std::string m_name;
		std::string m_path;
		unsigned m_count = 0;
		time_point m_time0, m_time1;
		
	public:
		section(std::string name_, std::string path_) : m_name(name_), m_path(path_) { }

		section(const section &) = delete;
		section & operator=(const section &) = delete;

		section(section &&) = default;
		section & operator=(section &&) = default;
		
		const std::string & name() const {
			return m_name;
		}

		const std::string & path() const {
			return m_path;
		}

		unsigned count() const {
			return m_count;
		}

		const time_point & time0() const {
			return m_time0;
		}

		const time_point & time1() const {
			return m_time1;
		}

		static const section * current() noexcept;

		static bool threadProfiling();
		static void threadProfiling(bool);

		static bool defaultProfiling();
		static void defaultProfiling(bool);

	};

}

#endif // GECOM_SECTION_HPP
