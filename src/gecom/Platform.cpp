
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


#include <windows.h>
#include <ImageHlp.h>
#include <Psapi.h>

#include <iostream>
#include <cassert>
#include <utility>
#include <vector>
#include <mutex>
#include <atomic>
#include <type_traits>

namespace {
	
	using namespace gecom::platform;

	template <typename T, typename U, typename V>
	T reinterpret_rva_cast(U *base, V rva) {
		return reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(base) + ptrdiff_t(rva));
	}

	template <typename T, typename U, typename V>
	const T reinterpret_rva_cast(const U *base, V rva) {
		return reinterpret_cast<T *>(reinterpret_cast<const unsigned char *>(base) + ptrdiff_t(rva));
	}

	template <typename ResultT, typename ...ArgTR>
	auto namedModuleProcAsType(ResultT (__stdcall *proctype)(ArgTR ...), const std::string &modname, const std::string procname) {
		HMODULE hmod = GetModuleHandleA(modname.c_str());
		if (hmod == INVALID_HANDLE_VALUE) throwLastError("GetModuleHandle");
		FARPROC dllproc = GetProcAddress(hmod, procname.c_str());
		if (!dllproc) gecom::throwLastError("GetProcAddress");
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
				throwLastError("EnumProcessModules");
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
		// http://win32assembly.programminghorizon.com/pe-tut7.html

	}

	void ** importedProcAddressAddress(HMODULE hmod, const std::string &impmodname, const std::string &impprocname) {
		// http://vxheaven.org/lib/vhf01.html
		// http://win32assembly.programminghorizon.com/pe-tut6.html
		// get import descriptors for target module
		ULONG entrysize = 0;
		auto importdesc = PIMAGE_IMPORT_DESCRIPTOR(
			// ImageDirectoryEntryToData is supposedly not threadsafe
			ImageDirectoryEntryToData(hmod, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &entrysize)
		);
		// not all modules have an import section
		if (!importdesc) {
			char modfilename[MAX_PATH];
			GetModuleFileNameA(hmod, modfilename, MAX_PATH);
			std::cerr << "failed to get import descriptors for " << modfilename << std::endl;
			return;
		}
		// find import descriptors for origin module
		for (; importdesc->Name; ++importdesc) {
			const char *modname = reinterpret_rva_cast<const char *>(hmod, importdesc->Name);
			if (impmodname == modname && importdesc->OriginalFirstThunk) {
				// array of import pointers
				auto name_thunk = reinterpret_rva_cast<PIMAGE_THUNK_DATA>(hmod, importdesc->OriginalFirstThunk);
				// array of function pointers
				auto proc_thunk = reinterpret_rva_cast<PIMAGE_THUNK_DATA>(hmod, importdesc->FirstThunk);
				// search for function name
				for (; name_thunk->u1.AddressOfData; ++name_thunk, ++proc_thunk) {
					// skip functions imported by ordinal only
					if (name_thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
					// TODO is this pointer or RVA?
					auto import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(name_thunk->u1.AddressOfData);
					if (impprocname == static_cast<const char *>(import->Name)) {
						// gotcha!
						return reinterpret_cast<void **>(&proc_thunk->u1.Function);
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
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, m_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);
			buf[sizeof(buf) - 1] = '\0';
			if (!hint_.empty()) {
				m_what += hint_;
				m_what += ": ";
			}
			m_what += buf;
		}

		const char * win32_error::what() const noexcept {
			return m_what.c_str();
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
			std::cerr << e.what() << std::endl;
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
