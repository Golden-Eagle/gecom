
#include <cassert>
#include <atomic>
#include <vector>
#include <memory>

#include "Section.hpp"

namespace {
	std::atomic<bool> default_profiling { false };
	thread_local bool thread_profiling { default_profiling };

	// this does not get leaked if sections are used correctly
	thread_local std::vector<gecom::section> *sections = nullptr;

	std::string currentPath() {
		std::string r;
		if (!sections) return r;
		for (const auto &s : *sections) {
			r += s.name();
			r += '/';
		}
		return r;
	}
}

namespace gecom {

	section_guard::section_guard(std::string name_) : m_entered(false), m_name(std::move(name_)) {
		if (!sections) {
			sections = new std::vector<section>();
		}
		if (sections->empty() || sections->back().name() != m_name) {
			std::string path = currentPath();
			path += m_name;
			path += '/';
			sections->push_back({ m_name, std::move(path) });
			if (thread_profiling) {
				sections->back().m_time0 = section::clock::now();
				// TODO call to profiler?
			}
		}
		m_entered = true;
		assert(!sections->empty());
		++sections->back().m_count;
	}

	section_guard::~section_guard() {
		if (m_entered) {
			assert(!sections->empty());
			assert(sections->back().name() == m_name);
			if (--sections->back().m_count == 0) {
				if (thread_profiling) {
					sections->back().m_time1 = section::clock::now();
					// TODO call to profiler?
				}
				sections->pop_back();
			}
			if (sections->empty()) {
				delete sections;
				sections = nullptr;
			}
		}
		
	}

	const section * section::current() noexcept {
		return sections ? (sections->empty() ? nullptr : &sections->back()) : nullptr;
	}

	bool section::threadProfiling() {
		return thread_profiling;
	}

	void section::threadProfiling(bool b) {
		thread_profiling = b;
	}

	bool section::defaultProfiling() {
		return default_profiling;
	}

	void section::defaultProfiling(bool b) {
		default_profiling = b;
	}

}
