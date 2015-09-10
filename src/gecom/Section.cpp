
#include <cassert>
#include <atomic>
#include <vector>
#include <memory>

#include "Section.hpp"

namespace {
	
	struct SectionStatics {
		std::atomic<bool> default_profiling { false };
	};

	auto & sectionStatics() {
		static SectionStatics s;
		return s;
	}

	struct SectionTLStatics {
		bool current_profiling { sectionStatics().default_profiling };

		// this does not get leaked if sections are used correctly
		// this is a pointer because msvc doesn't like it otherwise
		std::vector<gecom::section> *sections = nullptr;
	};

	auto & sectionTLStatics() {
		static thread_local SectionTLStatics s;
		return s;
	}

	std::string currentPath() {
		std::string r;
		auto sections = sectionTLStatics().sections;
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
		auto &sections = sectionTLStatics().sections;
		if (!sections) {
			sections = new std::vector<section>();
		}
		if (sections->empty() || sections->back().name() != m_name) {
			std::string path = currentPath();
			path += m_name;
			path += '/';
			sections->push_back({ m_name, std::move(path) });
			if (section::currentProfiling()) {
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
			auto &sections = sectionTLStatics().sections;
			assert(!sections->empty());
			assert(sections->back().name() == m_name);
			if (--sections->back().m_count == 0) {
				if (section::currentProfiling()) {
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
		auto sections = sectionTLStatics().sections;
		return sections ? (sections->empty() ? nullptr : &sections->back()) : nullptr;
	}

	bool section::currentProfiling() {
		return sectionTLStatics().current_profiling;
	}

	void section::currentProfiling(bool b) {
		sectionTLStatics().current_profiling = b;
	}

	bool section::defaultProfiling() {
		return sectionStatics().default_profiling;
	}

	void section::defaultProfiling(bool b) {
		sectionStatics().default_profiling = b;
	}

	size_t SectionInit::refcount = 0;

	SectionInit::SectionInit() {
		if (refcount++ == 0) {
			// ensure section statics are initialized
			sectionStatics();
			sectionTLStatics();
		}
	}

	SectionInit::~SectionInit() {
		if (--refcount == 0) {
			// section statics about to be destroyed
			assert(!sectionTLStatics().sections);
		}
	}

}
