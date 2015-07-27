
#ifndef GECOM_SECTION_HPP
#define GECOM_SECTION_HPP

#include <utility>
#include <string>
#include <chrono>

namespace gecom {

	class Section {
	public:
		using clock = std::chrono::high_resolution_clock;
		using time_point = clock::time_point;

		struct section_entry {
			std::string name;
			std::string path;
			time_point time0, time1;
		};

	private:
		const Section *m_parent = nullptr;
		section_entry m_entry;

		void enter(std::string name);
		void exit() noexcept;

	public:
		Section(const Section &) = delete;
		Section(Section &&) = delete;
		Section & operator=(const Section &) = delete;
		Section & operator=(Section &&) = delete;

		Section(std::string name_) {
			enter(std::move(name_));
		}

		const std::string & name() const {
			return m_entry.name;
		}

		const std::string & path() const {
			return m_entry.path;
		}

		const section_entry & entry() const {
			return m_entry;
		}

		const Section * parent() const noexcept {
			return m_parent;
		}

		const Section * root() const noexcept {
			const Section *r = parent();
			while (r) {
				r = r->parent();
			}
			return r;
		}

		~Section() {
			exit();
		}

		static const Section * current() noexcept;

		static bool threadProfiling();
		static void threadProfiling(bool);

		static bool defaultProfiling();
		static void defaultProfiling(bool);

	};

}

#endif // GECOM_SECTION_HPP
