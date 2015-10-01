
#include "Platform.hpp"

namespace gecom {
	size_t PlatformInit::refcount = 0;

	void onPlatformInit();
	void onPlatformTerminate();

	PlatformInit::PlatformInit() {
		if (refcount++ == 0) {
			onPlatformInit();
		}
	}

	PlatformInit::~PlatformInit() {
		if (--refcount == 0) {
			onPlatformTerminate();
		}
	}
}

#if defined(GECOM_PLATFORM_WIN32)

#include <cstdio>

#include <windows.h>
#include <ImageHlp.h>
#include <Psapi.h>

#include <cassert>
#include <utility>
#include <vector>
#include <mutex>
#include <atomic>
#include <type_traits>

namespace {
	
	using namespace gecom::platform;

	template <typename ResultT, typename ...ArgTR>
	auto namedModuleProcAsType(ResultT (__stdcall *proctype)(ArgTR ...), const std::string &modname, const std::string procname) {
		HMODULE hmod = GetModuleHandleA(modname.c_str());
		if (hmod == INVALID_HANDLE_VALUE) throwLastError("get module handle");
		FARPROC dllproc = GetProcAddress(hmod, procname.c_str());
		if (!dllproc) gecom::throwLastError("get proc address");
		return reinterpret_cast<decltype(proctype)>(dllproc);
	}

	HMODULE getModuleHandleByAddress(void *p, bool increfcount = false) {
		HMODULE h = nullptr;
		GetModuleHandleExA(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			(increfcount ? 0 : GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT),
			LPCSTR(p),
			&h
		);
		return h;
	}

	std::vector<HMODULE> grabLoadedModules() {
		// list loaded modules
		std::vector<HMODULE> modlist;
		DWORD bytesneeded = 0;
		DWORD modcount = 128;
		do {
			modlist.resize(modcount, nullptr);
			if (!EnumProcessModulesEx(
				GetCurrentProcess(),
				modlist.data(),
				sizeof(HMODULE) * modlist.size(),
				&bytesneeded,
				LIST_MODULES_ALL
			)) {
				throwLastError("list modules");
			}
			modcount = bytesneeded / sizeof(HMODULE);
		} while (modcount > modlist.size());
		modlist.resize(modcount);
		// increment reference count to valid modules
		for (auto it = modlist.begin(); it != modlist.end(); ) {
			HMODULE h = *it;
			// try inc refcount (this ensures the handle is for _a_ valid module,
			// not necessarily the same one as when EnumProcessModules was called)
			if (h == getModuleHandleByAddress(h, true)) {
				// module is good
				++it;
			} else {
				// module has been unloaded
				it = modlist.erase(it);
			}
		}
		// returned handles are real, safe to use FreeLibrary
		return modlist;
	}

	void ** exportedProcRVAAddress(HMODULE hmod, const std::string &procname) {

	}

	void ** importedProcAddressAddress(HMODULE hmod, const std::string &impmodname, const std::string &procname) {

	}

	bool verifyImportedProcAddresses(const std::string &procname, const void *proc) {

	}

	void hookImportedProcInModule(HMODULE hmod, void *dllproc, void *newproc) {
		// synchronize to prevent VirtualProtect race conditions on our part
		// also, ImageDirectoryEntryToData is supposedly not threadsafe
		// TODO is it possible to avoid VirtualProtect race conditions with 3rd-party mechanisms?
		static std::mutex image_mutex;
		std::unique_lock<std::mutex> lock(image_mutex);
		// http://vxheaven.org/lib/vhf01.html
		// get import descriptors for target module
		char modfilename[MAX_PATH];
		GetModuleFileNameA(hmod, modfilename, MAX_PATH);
		fprintf(stderr, "installing hook in module %p [%s]...\n", hmod, modfilename);
		ULONG entrysize = 0;
		auto importdesc = PIMAGE_IMPORT_DESCRIPTOR(
			ImageDirectoryEntryToData(hmod, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &entrysize)
		);
		if (!importdesc) {
			//gecom::throwLastError("image import descriptors");
			fprintf(stderr, "failed to get import descriptors\n");
			return;
		}
		// get a handle to the module the target function was exported from
		HMODULE hmod_origin = getModuleHandleByAddress(dllproc);
		// find import descriptors for origin module
		for (; importdesc->Name; ++importdesc) {
			auto thunk = PIMAGE_THUNK_DATA(PBYTE(hmod) + importdesc->FirstThunk);
			if (!thunk->u1.Function) continue;
			HMODULE hmod_imp = getModuleHandleByAddress(PVOID(thunk->u1.Function));
			if (hmod_imp == hmod_origin) {
				// import descriptor is for origin module; search for target function
				for (; thunk->u1.Function; ++thunk) {
					PROC *pproc = reinterpret_cast<PROC *>(&thunk->u1.Function);
					if (*pproc == PROC(dllproc)) {
						// gotcha!
						// get memory info for region around IAT entry
						MEMORY_BASIC_INFORMATION mbi;
						if (!VirtualQuery(pproc, &mbi, sizeof(MEMORY_BASIC_INFORMATION))) {
							throwLastError("get memory info");
						}
						// un-write-protect
						if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect)) {
							throwLastError("virtual protect");
						}
						// update IAT entry
						InterlockedExchangePointer(reinterpret_cast<void * volatile *>(pproc), newproc);
						// restore write-protection
						if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect)) {
							throwLastError("virtual protect");
						}
						// and we're done
						fprintf(stderr, "hooked %p in module %p\n", dllproc, hmod);
						break;
					}
				}
			}
		}
	}




}

namespace gecom {

	namespace platform {

		win32_error::win32_error(int err_, const std::string &hint_) : m_err(err_) {
			char buf[256];
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);
			buf[sizeof(buf) - 1] = '\0';
			if (!hint_.empty()) {
				m_what += hint_;
				m_what += ": ";
			}
			m_what += buf;
		}

		const char * win32_error::what() const noexcept {
			return &m_what.front();
		}

		void throwLastError(const std::string & hint) {
			throw win32_error(GetLastError(), hint);
		}

		void hookImportedProc(const std::string &modname, const std::string &procname, const void *newproc, void *&oldproc) {
			// synchronize
			// load library
			// get export RVA address
			// verify export/import address consistency in loaded modules
			// while (export RVA not ours || verification failed):
			//   do:
			//     write export address to oldproc
			//   until cmpxchg export RVA with (newproc - hmod)
			//   replace import addresses in loaded modules

		}

	}

	void onPlatformInit() {
		try {

			
		} catch (platform::win32_error &e) {
			fprintf(stderr, "%s\n", e.what());
			
		}
	}

	void onPlatformTerminate() {

	}
}

#elif defined(GECOM_PLATFORM_POSIX)

namespace gecom {

	void throwLastError(const std::string &hint) {
		throw std::runtime_error("not supported on posix yet");
	}

}

#else

namespace gecom {
	
	void throwLastError(const std::string &hint) {
		throw std::runtime_error("not supported on this platform");
	}

}

#endif // GECOM_PLATFORM_*
