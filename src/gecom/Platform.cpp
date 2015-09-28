
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

	void hookImportedProcInAllModules(void *dllproc, void *newproc) {
		// list all loaded modules (this includes the current process)
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
		// hook all loaded modules
		// if a module is loaded/unloaded after EnumProcessModules,
		// we obviously can't insert hooks in it
		for (HMODULE h : modlist) {
			// try inc refcount (this ensures the handle is for _a_ valid module,
			// not necessarily the same one as when EnumProcessModules was called)
			if (h = getModuleHandleByAddress(h, true)) {
				// hook
				hookImportedProcInModule(h, dllproc, newproc);
				// dec refcount
				FreeLibrary(h);
			}
		}
	}

	class HookedLibraryLoader {
	private:
		struct hook {
			void *dllproc;
			void *newproc;
		};

		struct Statics {
			std::mutex hook_mutex;
			std::vector<hook> hooks;
			// original address of GetProcAddress
			decltype(&GetProcAddress) get_proc_address_org;
			// original address of GetModuleHandleExA
			decltype(&GetModuleHandleExA) get_module_handle_exa_org;
			// original address of LoadLibraryExA
			decltype(&LoadLibraryExA) load_library_exa_org;
			// original address of GetModuleHandleExW
			decltype(&GetModuleHandleExW) get_module_handle_exw_org;
			// original address of LoadLibraryExW
			decltype(&LoadLibraryExW) load_library_exw_org;
		};

		static auto & statics() {
			static Statics s;
			return s;
		}

		static void hookAllModules() {
			std::unique_lock<std::mutex> lock(statics().hook_mutex);
			for (const auto &h : statics().hooks) {
				hookImportedProcInAllModules(h.dllproc, h.newproc);
			}
		}

	public:
		static void addHook(void *dllproc, void *newproc) {
			std::unique_lock<std::mutex> lock(statics().hook_mutex);
			hook h { dllproc, newproc };
			statics().hooks.push_back(std::move(h));
		}

		static FARPROC __stdcall getProcAddress(HMODULE hModule, LPCSTR lpProcName) {
			std::unique_lock<std::mutex> lock(statics().hook_mutex);
			FARPROC r = statics().get_proc_address_org(hModule, lpProcName);
			for (const auto &h : statics().hooks) {
				if (r == FARPROC(h.dllproc)) return FARPROC(h.newproc);
			}
			return r;
		}

		static HMODULE __stdcall loadLibraryA(LPCSTR lpLibFileName) {
			// this is getting spammed by SwapBuffers
			return loadLibraryExA(lpLibFileName, nullptr, 0);
		}

		static HMODULE __stdcall loadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
			// try get handle to already loaded module (this increments the refcount)
			HMODULE hmod = nullptr;
			if (statics().get_module_handle_exa_org(0, lpLibFileName, &hmod)) return hmod;
			// load module
			hmod = statics().load_library_exa_org(lpLibFileName, hFile, dwFlags);

			char modfilename[MAX_PATH];
			GetModuleFileNameA(hmod, modfilename, MAX_PATH);
			fprintf(stderr, "loaded module %p [%s]...\n", hmod, modfilename);

			// install hooks, ensuring we get all modules loaded by this one too
			hookAllModules();

			return hmod;
		}

		static HMODULE __stdcall loadLibraryW(LPCWSTR lpLibFileName) {
			return loadLibraryExW(lpLibFileName, nullptr, 0);
		}

		static HMODULE __stdcall loadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
			// try get handle to already loaded module (this increments the refcount)
			HMODULE hmod = nullptr;
			if (statics().get_module_handle_exw_org(0, lpLibFileName, &hmod)) return hmod;
			// load module
			hmod = statics().load_library_exw_org(lpLibFileName, hFile, dwFlags);
			// install hooks, ensuring we get all modules loaded by this one too
			hookAllModules();
			return hmod;
		}

		static void initHooks() {
			// hook GetProcAddress so it returns our proc addresses for hooked functions
			hookImportedProc("kernel32.dll", "GetProcAddress", HookedLibraryLoader::getProcAddress);
			// hook LoadLibrary so we can install hooks in newly-loaded modules
			hookImportedProc("kernel32.dll", "LoadLibraryA", HookedLibraryLoader::loadLibraryA);
			hookImportedProc("kernel32.dll", "LoadLibraryExA", HookedLibraryLoader::loadLibraryExA);
			hookImportedProc("kernel32.dll", "LoadLibraryW", HookedLibraryLoader::loadLibraryW);
			hookImportedProc("kernel32.dll", "LoadLibraryExW", HookedLibraryLoader::loadLibraryExW);
		}
	};



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

		void * hookImportedProc(void *dllproc, void *newproc) {
			// sanity check
			if (!dllproc) throw std::invalid_argument("bad target proc address");
			// add to LoadLibrary hook list
			HookedLibraryLoader::addHook(dllproc, newproc);
			// hook loaded modules
			hookImportedProcInAllModules(dllproc, newproc);
			return dllproc;
		}

		void * hookImportedProc(const std::string &modname, const std::string &procname, void *newproc) {
			void *dllproc = namedModuleProcAsType(FARPROC(), modname, procname);
			hookImportedProc(dllproc, newproc);
			return dllproc;
		}


	}

	void onPlatformInit() {
		try {
			HookedLibraryLoader::initHooks();
			
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

	void * hookImportedProc(void *dllproc, void *newproc) {
		throw std::runtime_error("not supported on posix yet");
	}

	void * hookImportedProc(const std::string &modname, const std::string &procname, void *newproc) {
		throw std::runtime_error("not supported on posix yet");
	}

}

#else

namespace gecom {
	
	void throwLastError(const std::string &hint) {
		throw std::runtime_error("not supported on this platform");
	}

	void * hookImportedProc(void *dllproc, void *newproc) {
		throw std::runtime_error("not supported on this platform");
	}

	void * hookImportedProc(const std::string &modname, const std::string &procname, void *newproc) {
		throw std::runtime_error("not supported on this platform");
	}

}

#endif // GECOM_PLATFORM_*
