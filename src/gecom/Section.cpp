
#include <atomic>

#include "Section.hpp"

namespace {
	std::atomic<bool> default_profiling { false };
	thread_local bool thread_profiling { default_profiling };
	thread_local const gecom::Section *current_section = nullptr;

	void sectionPath(std::string &path, const gecom::Section *s) {
		if (const gecom::Section *p = s->parent()) {
			sectionPath(path, p);
			path += '/';
		}
		path += s->name();
	}
}

namespace gecom {

	void Section::enter(std::string name) {
		m_entry.name = name;
		m_entry.path.clear();
		sectionPath(m_entry.path, this);
		if (thread_profiling) {
			m_entry.time0 = clock::now();
		}
		m_parent = current_section;
		current_section = this;
	}

	void Section::exit() noexcept {
		current_section = m_parent;
		if (thread_profiling) {
			m_entry.time1 = clock::now();
			// TODO call to profiler
		}
	}

	const Section * Section::current() noexcept {
		return current_section;
	}

	bool Section::threadProfiling() {
		return thread_profiling;
	}

	void Section::threadProfiling(bool b) {
		thread_profiling = b;
	}

	bool Section::defaultProfiling() {
		return default_profiling;
	}

	void Section::defaultProfiling(bool b) {
		default_profiling = b;
	}

}
