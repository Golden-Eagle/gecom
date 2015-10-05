
#include "Platform.hpp"

#include "Util.hpp"

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

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>
#include <mutex>
#include <atomic>
#include <type_traits>

namespace {
	
	using namespace gecom::platform;

	template <typename T, typename U, typename V>
	T reinterpret_rva_cast(U *base, V rva) {
		return reinterpret_cast<T>(reinterpret_cast<unsigned char *>(base) + ptrdiff_t(rva));
	}

	template <typename T, typename U, typename V>
	T reinterpret_rva_cast(const U *base, V rva) {
		return reinterpret_cast<T>(reinterpret_cast<const unsigned char *>(base) + ptrdiff_t(rva));
	}

	class virtualprotect_guard {
	private:
		volatile void *m_base = nullptr;
		size_t m_size = 0;
		DWORD m_old_protect;

		void destroy() {
			if (m_base) {
				// restore original protection
				if (!VirtualProtect(const_cast<void *>(m_base), m_size, m_old_protect, &m_old_protect)) {
					// this is kinda bad
					std::cerr << "VirtualProtect failed to restore original memory protection at " << m_base << " [" << m_size << " bytes], aborting" << std::endl;
					std::abort();
				}
			}
		}

	public:
		virtualprotect_guard() { }

		virtualprotect_guard(volatile void *base_, size_t size_, DWORD protect_) : m_base(base_), m_size(size_) {
			// set new protection, store original protection
			if (!VirtualProtect(const_cast<void *>(m_base), m_size, protect_, &m_old_protect)) {
				throwLastError();
			}
		}

		virtualprotect_guard(const virtualprotect_guard &) = delete;
		virtualprotect_guard & operator=(const virtualprotect_guard &) = delete;

		virtualprotect_guard(virtualprotect_guard &&other) noexcept :
			m_base(other.m_base),
			m_size(other.m_size),
			m_old_protect(other.m_old_protect)
		{
			other.m_base = nullptr;
			other.m_size = 0;
		}

		virtualprotect_guard & operator=(virtualprotect_guard &&other) noexcept {
			destroy();
			m_base = other.m_base;
			m_size = other.m_size;
			m_old_protect = other.m_old_protect;
			other.m_base = nullptr;
			other.m_size = 0;
			return *this;
		}

		~virtualprotect_guard() {
			destroy();
		}
	};

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

	std::vector<module_handle> grabLoadedModules() {
		// list loaded modules
		std::vector<HMODULE> modlist_temp;
		DWORD bytesneeded = 0;
		DWORD modcount = 128;
		do {
			modlist_temp.resize(modcount, nullptr);
			if (!EnumProcessModulesEx(
				GetCurrentProcess(),
				modlist_temp.data(),
				DWORD(sizeof(HMODULE) * modlist_temp.size()),
				&bytesneeded,
				LIST_MODULES_ALL
			)) {
				throwLastError("EnumProcessModules");
			}
			modcount = bytesneeded / sizeof(HMODULE);
		} while (modcount > modlist_temp.size());
		modlist_temp.resize(modcount);
		// increment reference count to valid modules, construct handle objects
		std::vector<module_handle> modlist;
		modlist.reserve(modlist_temp.size());
		for (auto h : modlist_temp) {
			// try inc refcount (this ensures the handle is for _a_ valid module,
			// not necessarily the same one as when EnumProcessModules was called)
			if (h != getModuleHandleByAddress(h, true)) continue;
			// module is good
			modlist.emplace_back(h);
		}
		// returned handles are 'real'
		return modlist;
	}

	PDWORD exportedProcRVAAddress(HMODULE hmod, const std::string &procname) {
		// http://win32assembly.programminghorizon.com/pe-tut7.html
		// default error state
		SetLastError(ERROR_PROC_NOT_FOUND);
		// get export table for target module
		ULONG entrysize = 0;
		auto exportdir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
			// ImageDirectoryEntryToData is supposedly not threadsafe
			ImageDirectoryEntryToData(hmod, true, IMAGE_DIRECTORY_ENTRY_EXPORT, &entrysize)
		);
		// not all modules have an export section
		if (!exportdir) {
			char modfilename[MAX_PATH];
			GetModuleFileNameA(hmod, modfilename, MAX_PATH);
			std::cerr << "failed to get export table for " << modfilename << std::endl;
			return nullptr;
		}
		// array of function RVAs (as DWORDs)
		auto functions = reinterpret_rva_cast<PDWORD>(hmod, exportdir->AddressOfFunctions);
		// array of named function name RVAS (as DWORDs)
		auto names = reinterpret_rva_cast<PDWORD>(hmod, exportdir->AddressOfNames);
		// array of named function indices
		auto named_indices = reinterpret_rva_cast<PWORD>(hmod, exportdir->AddressOfNameOrdinals);
		// search for function name
		for (DWORD i = 0; i < exportdir->NumberOfNames; ++i) {
			auto expprocname = reinterpret_rva_cast<const char *>(hmod, names[i]);
			if (procname == expprocname) {
				// check function index
				if (named_indices[i] < exportdir->NumberOfFunctions) {
					// gotcha!
					SetLastError(NOERROR);
					return functions + named_indices[i];
				} else {
					return nullptr;
				}
			}
		}
		// didn't find exported function
		return nullptr;
	}

	const void ** importedProcAddressAddress(HMODULE hmod, const std::string &modname, const std::string &procname) {
		// http://vxheaven.org/lib/vhf01.html
		// http://win32assembly.programminghorizon.com/pe-tut6.html
		// default error state
		SetLastError(ERROR_PROC_NOT_FOUND);
		// get import descriptors for target module
		ULONG entrysize = 0;
		auto importdesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
			// ImageDirectoryEntryToData is supposedly not threadsafe
			ImageDirectoryEntryToData(hmod, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &entrysize)
		);
		// not all modules have an import section
		if (!importdesc) {
			char modfilename[MAX_PATH];
			GetModuleFileNameA(hmod, modfilename, MAX_PATH);
			std::cerr << "failed to get import descriptors for " << modfilename << std::endl;
			return nullptr;
		}
		// find import descriptors for origin module
		for (; importdesc->Name; ++importdesc) {
			const char *impmodname = reinterpret_rva_cast<const char *>(hmod, importdesc->Name);
			// making sure to use case-insensitive comparison for module names
			if (stricmp(modname.c_str(), impmodname) == 0 && importdesc->OriginalFirstThunk) {
				// array of import RVAs
				auto name_thunk = reinterpret_rva_cast<PIMAGE_THUNK_DATA>(hmod, importdesc->OriginalFirstThunk);
				// array of function pointers
				auto proc_thunk = reinterpret_rva_cast<PIMAGE_THUNK_DATA>(hmod, importdesc->FirstThunk);
				// search for function name
				for (; name_thunk->u1.AddressOfData; ++name_thunk, ++proc_thunk) {
					// skip functions imported by ordinal only
					if (name_thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
					// pointer to import name struct (AddressOfData is an RVA in this context, _not_ a pointer)
					auto impprocdata = reinterpret_rva_cast<PIMAGE_IMPORT_BY_NAME>(hmod, name_thunk->u1.AddressOfData);
					if (procname == static_cast<const char *>(impprocdata->Name)) {
						// gotcha!
						SetLastError(NOERROR);
						return reinterpret_cast<const void **>(&proc_thunk->u1.Function);
					}
				}
			}
		}
		// didn't find imported function
		return nullptr;
	}

	decltype(&LoadLibraryA) old_load_library_a = nullptr;
	decltype(&LoadLibraryExA) old_load_library_exa = nullptr;

	HMODULE __stdcall loadLibraryAHook(LPCSTR lpFileName) {
		// this gets spammed by some WGL things
		std::cerr << "LoadLibraryA: " << lpFileName << std::endl;
		return old_load_library_a(lpFileName);
	}

	HANDLE __stdcall loadLibraryExAHook(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags) {
		std::cerr << "LoadLibraryExA: " << lpFileName << std::endl;
		return old_load_library_exa(lpFileName, hFile, dwFlags);
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

		module_handle::module_handle(const std::string &modname_) {
			HMODULE h = LoadLibraryA(modname_.c_str());
			if (h == INVALID_HANDLE_VALUE) throwLastError("LoadLibrary");
			m_hmod = h;
		}

		void module_handle::destroy() noexcept {
			if (m_hmod) {
				FreeLibrary(HMODULE(m_hmod));
			}
		}

		void * module_handle::procAddress(const std::string &procname) const {
			void *proc = GetProcAddress(HMODULE(m_hmod), procname.c_str());
			if (!proc) throwLastError();
			return proc;
		}

		void throwLastError(const std::string & hint) {
			throw win32_error(GetLastError(), hint);
		}

		void hookImportedProc(const std::string &modname, const std::string &procname, const void *newproc, const void **oldproc) {
			// synchronize, this procedure isn't really threadsafe
			static std::mutex hook_mutex;
			std::unique_lock<std::mutex> lock(hook_mutex);

			// load module with function to be hooked
			module_handle expmod(modname);

			// exported proc RVA address
			DWORD volatile * const pexprva = exportedProcRVAAddress(HMODULE(expmod.nativeHandle()), procname);
			if (!pexprva) throwLastError();
			
			// export/import address verification status
			bool verified = false;

			do {
				// get exported proc address
				DWORD oldexprva = *pexprva;
				const void *oldexpproc = reinterpret_rva_cast<void *>(HMODULE(expmod.nativeHandle()), oldexprva);

				// if exported proc address not ours, try cmpexchg
				if (oldexpproc != newproc) {
					// store old proc address
					*oldproc = oldexpproc;
					// calculate new RVA
					ptrdiff_t newexprva = reinterpret_cast<const unsigned char *>(newproc) -
						reinterpret_cast<const unsigned char *>(HMODULE(expmod.nativeHandle()));
					// RVA is a DWORD, check we're in range
					if (ptrdiff_t(DWORD(newexprva)) != newexprva) {
						throw std::runtime_error("new proc address not within DWORD range of module");
					}
					// unprotect and write
					virtualprotect_guard vpg(reinterpret_cast<volatile void *>(pexprva), sizeof(void *), PAGE_READWRITE);
					// if cmpexchg failed, bail and try again
					if (InterlockedCompareExchange(pexprva, DWORD(newexprva), oldexprva) != oldexprva) continue;
				}

				// grab loaded modules
				auto mods0 = grabLoadedModules();

				// replace imported proc addresses
				for (const auto &mod : mods0) {
					// get pointer to imported proc address
					const void * volatile * const pproc = importedProcAddressAddress(HMODULE(mod.nativeHandle()), modname, procname);
					if (!pproc) continue;
					// unprotect and write
					virtualprotect_guard vpg(reinterpret_cast<volatile void *>(pproc), sizeof(void *), PAGE_READWRITE);
					InterlockedExchangePointer(const_cast<void * volatile *>(pproc), const_cast<void *>(newproc));
				}

				// grab loaded modules again
				// any differences are newly loaded modules, which should have loaded our replaced export address
				auto mods1 = grabLoadedModules();

				// begin verification
				verified = true;

				// verify exported proc address
				DWORD newexprva = *pexprva;
				void *newexpproc = reinterpret_rva_cast<void *>(HMODULE(expmod.nativeHandle()), newexprva);
				verified &= newexpproc == newproc;

				// verify imported proc addresses
				for (const auto &mod : mods1) {
					// get pointer to imported proc address
					const void * volatile * const pproc = importedProcAddressAddress(HMODULE(mod.nativeHandle()), modname, procname);
					if (!pproc) continue;
					verified &= *pproc == newproc;
				}

			} while (!verified);

			// don't unload the module with our modified export table
			expmod.detach();
		}

	}

	void onPlatformInit() {
		try {
			// test...
			hookImportedProc("kernel32.dll", "LoadLibraryA", loadLibraryAHook, &old_load_library_a);
			hookImportedProc("kernel32.dll", "LoadLibraryExA", loadLibraryExAHook, &old_load_library_exa);

		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
	}

	void onPlatformTerminate() {

	}
}

#elif defined(GECOM_PLATFORM_POSIX)

namespace gecom {

	// TODO posix implementations

	void throwLastError(const std::string &hint) {
		throw std::runtime_error("not supported on posix yet");
	}

}

#else

namespace gecom {
	
	// TODO default implementations

	void throwLastError(const std::string &hint) {
		throw std::runtime_error("not supported on this platform");
	}

}

#endif // GECOM_PLATFORM_*
