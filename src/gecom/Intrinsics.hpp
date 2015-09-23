
/*
 * GECOM Intrinsics
 *
 * Detects and includes available compiler intrinsics, including platform specifics.
 * Provides wrappers for some platform-specific intrinsics with default implementations
 * if no intrinsic implementation is available.
 */

#ifndef GECOM_INTRINSICS_HPP
#define GECOM_INTRINSICS_HPP

// detect available platform-specific intrinsics
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
// Microsoft C/C++-compatible compiler, targeting x86/x86-64
#ifdef _M_IX86
// targeting x86
#define GECOM_FOUND_X86
#if _M_IX86_FP == 0
// targeting IA32, no SSE
#elif _M_IX86_FP == 1
// targeting SSE
#define GECOM_FOUND_SSE
#elif _M_IX86_FP == 2
// targeting SSE2
#define GECOM_FOUND_SSE
#define GECOM_FOUND_SSE2
#else
#error unknown value of _M_IX86_FP
#endif
#endif
#ifdef _M_X64
// targeting x64
#define GECOM_FOUND_X86
#define GECOM_FOUND_X64
// x64 requires SSE2
#define GECOM_FOUND_SSE
#define GECOM_FOUND_SSE2
#endif
#ifdef __AVX__
// targeting AVX; assume we have all (Intel) SSE
#define GECOM_FOUND_SSE3
#define GECOM_FOUND_SSSE3
#define GECOM_FOUND_SSE41
#define GECOM_FOUND_SSE42
#define GECOM_FOUND_AVX
#endif
#ifdef __AVX2__
// targeting AVX2
#define GECOM_FOUND_AVX2
#endif
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
// Microsoft C/C++-compatible compiler, targeting ARM/ARM64
// TODO msvc/arm
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
// GCC-compatible compiler, targeting x86/x86-64
#ifdef __i386__
// targeting x86
#define GECOM_FOUND_X86
#endif
#ifdef __x86_64__
// targeting x64
#define GECOM_FOUND_X86
#define GECOM_FOUND_X64
#endif
#ifdef __SSE__
#define GECOM_FOUND_SSE
#endif
#ifdef __SSE2__
#define GECOM_FOUND_SSE2
#endif
#ifdef __SSE3__
#define GECOM_FOUND_SSE3
#endif
#ifdef __SSSE3__
#define GECOM_FOUND_SSSE3
#endif
#ifdef __SSE4_1__
#define GECOM_FOUND_SSE41
#endif
#ifdef __SSE4_2__
#define GECOM_FOUND_SSE42
#endif
#ifdef __SSE4A__
#define GECOM_FOUND_SSE4A
#endif
#ifdef __AVX__
#define GECOM_FOUND_AVX
#endif
#ifdef __AVX2__
#define GECOM_FOUND_AVX2
#endif
#elif defined(__GNUC__) && (defined(__arm__) || defined(__aarch64__))
// GCC-compatible compiler, targeting ARM/ARM64
// TODO gcc/arm
#ifdef __ARM_NEON__
#define GECOM_FOUND_NEON
#endif
#endif

// enable detected intrinsics
#ifdef GECOM_FOUND_X86
#define GECOM_USE_X86
#endif
#ifdef GECOM_FOUND_X64
#define GECOM_USE_X64
#endif
#ifdef GECOM_FOUND_ARM
#define GECOM_USE_ARM
#endif
#ifdef GECOM_FOUND_ARM64
#define GECOM_USE_ARM64
#endif
#ifdef GECOM_FOUND_SSE
#define GECOM_USE_SSE
#endif
#ifdef GECOM_FOUND_SSE2
#define GECOM_USE_SSE2
#endif
#ifdef GECOM_FOUND_SSE3
#define GECOM_USE_SSE3
#endif
#ifdef GECOM_FOUND_SSSE3
#define GECOM_USE_SSSE3
#endif
#ifdef GECOM_FOUND_SSE41
#define GECOM_USE_SSE41
#endif
#ifdef GECOM_FOUND_SSE42
#define GECOM_USE_SSE42
#endif
#ifdef GECOM_FOUND_SSE4A
#define GECOM_USE_SSE4A
#endif
#ifdef GECOM_FOUND_AVX
#define GECOM_USE_AVX
#endif
#ifdef GECOM_FOUND_AVX2
#define GECOM_USE_AVX2
#endif
#ifdef GECOM_FOUND_NEON
#define GECOM_USE_NEON
#endif

// enable prerequisites to enabled intrinsics
#ifdef GECOM_USE_AVX2
#define GECOM_USE_AVX
#endif
#ifdef GECOM_USE_AVX
#define GECOM_USE_SSE42
#endif
#ifdef GECOM_USE_SSE42
#define GECOM_USE_SSE41
#endif
#ifdef GECOM_USE_SSE41
#define GECOM_USE_SSSE3
#endif
#ifdef GECOM_USE_SSE4A
#define GECOM_USE_SSE3
#endif
#ifdef GECOM_USE_SSSE3
#define GECOM_USE_SSE3
#endif
#ifdef GECOM_USE_SSE3
#define GECOM_USE_SSE2
#endif
#ifdef GECOM_USE_SSE2
#define GECOM_USE_SSE
#endif

// allow prevention of all intrinsic usage
#ifdef GECOM_NO_INTRIN
#define GECOM_NO_X86
#define GECOM_NO_X64
#define GECOM_NO_ARM
#define GECOM_NO_ARM64
#define GECOM_NO_SSE
#define GECOM_NO_SSE2
#define GECOM_NO_SSE3
#define GECOM_NO_SSSE3
#define GECOM_NO_SSE41
#define GECOM_NO_SSE42
#define GECOM_NO_SSE4A
#define GECOM_NO_AVX
#define GECOM_NO_AVX2
#define GECOM_NO_AVX512
#define GECOM_NO_NEON
#endif

// disable AVX in GCC on Windows; >16-byte stack alignment is not possible
#if defined(__GNUC__) && defined(_WIN32)
#define GECOM_NO_AVX
#define GECOM_NO_AVX2
#define GECOM_NO_AVX512
#endif

// allow prevention of specific intrinsic usage
#ifdef GECOM_NO_X86
#undef GECOM_USE_X86
#endif
#ifdef GECOM_NO_X64
#undef GECOM_USE_X64
#endif
#ifdef GECOM_NO_SSE
#undef GECOM_USE_SSE
#endif
#ifdef GECOM_NO_SSE2
#undef GECOM_USE_SSE2
#endif
#ifdef GECOM_NO_SSE3
#undef GECOM_USE_SSE3
#endif
#ifdef GECOM_NO_SSSE3
#undef GECOM_USE_SSSE3
#endif
#ifdef GECOM_NO_SSE41
#undef GECOM_USE_SSE41
#endif
#ifdef GECOM_NO_SSE42
#undef GECOM_USE_SSE42
#endif
#ifdef GECOM_NO_SSE4A
#undef GECOM_USE_SSE4A
#endif
#ifdef GECOM_NO_AVX
#undef GECOM_USE_AVX
#endif
#ifdef GECOM_NO_AVX2
#undef GECOM_USE_AVX2
#endif
#ifdef GECOM_NO_AVX512
#undef GECOM_USE_AVX512
#endif
#ifdef GECOM_NO_ARM
#undef GECOM_USE_ARM
#endif
#ifdef GECOM_NO_NEON
#undef GECOM_USE_NEON
#endif

// includes for platform-specific intrinsics
#ifdef _MSC_VER
#include <stdlib.h>
#include <intrin.h>
#endif
#ifdef __GNUC__
#ifdef GECOM_USE_X86
#include <x86intrin.h>
#endif
#endif

// includes for x86/64 SIMD intrinsics
#ifdef GECOM_USE_SSE
#include <xmmintrin.h>
#endif
#ifdef GECOM_USE_SSE2
#include <emmintrin.h>
#endif
#ifdef GECOM_USE_SSE3
#include <pmmintrin.h>
#endif
#ifdef GECOM_USE_SSSE3
#include <tmmintrin.h>
#endif
#ifdef GECOM_USE_SSE41
#include <smmintrin.h>
#endif
#ifdef GECOM_USE_SSE42
#include <nmmintrin.h>
#endif
#ifdef GECOM_USE_SSE4A
#include <ammintrin.h>
#endif
#ifdef GECOM_USE_AVX
#include <immintrin.h>
#endif
#ifdef GECOM_USE_AVX2
#include <immintrin.h>
#endif
#ifdef GECOM_USE_AVX512
// ???
#endif

// includes for ARM SIMD intrinics
#ifdef GECOM_USE_NEON
#include <arm_neon.h>
#endif

#include <cstdint>
#include <cmath>
#include <type_traits>

namespace gecom {

	namespace intrin {

		namespace detail {

			template <typename UIntT, typename = std::enable_if_t<std::is_integral<UIntT>::value && std::is_unsigned<UIntT>::value>>
			inline UIntT byteSwapImpl(UIntT x) {
				static constexpr UIntT mask = (UIntT(1) << CHAR_BIT) - 1;
				UIntT r = 0;
				for (size_t i = 0; i < sizeof(UIntT); ++i) {
					r <<= CHAR_BIT;
					r |= x & mask;
					x >>= CHAR_BIT;
				}
				return r;
			}

#if defined(_MSC_VER)
			inline unsigned short byteSwapImpl(unsigned short x) {
				return _byteswap_ushort(x);
			}

			inline unsigned long byteSwapImpl(unsigned long x) {
				return _byteswap_ulong(x);
			}

			inline unsigned long long byteSwapImpl(unsigned long long x) {
				return _byteswap_uint64(x);
			}
#elif defined(__GNUC__)
			inline uint16_t byteSwapImpl(uint16_t x) {
				return __builtin_bswap16(x);
			}

			inline uint32_t byteSwapImpl(uint32_t x) {
				return __builtin_bswap32(x);
			}

			inline uint64_t byteSwapImpl(uint64_t x) {
				return __builtin_bswap64(x);
			}
#endif
		}

		// swap order of bytes (0xaabb -> 0xbbaa)
		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		inline IntT byteSwap(IntT x) {
			using UIntT = std::make_unsigned_t<IntT>;
			UIntT r = detail::byteSwapImpl(reinterpret_cast<UIntT &>(x));
			return reinterpret_cast<IntT &>(r);
		}

		namespace detail {

			template <typename UIntT, typename = std::enable_if_t<std::is_integral<UIntT>::value && std::is_unsigned<UIntT>::value>>
			inline unsigned bitScanForwardImpl(UIntT x) {
				int i = 0;
				for (; x && !(x & 0x1); x >>= 1, ++i);
				return i;
			}

#if defined(_MSC_VER)
#ifdef GECOM_USE_X86
			// native x86 bitscan
			inline unsigned bitScanReverseImplbitScanForwardImpl(unsigned long x) {
				unsigned long r;
				_BitScanForward(&r, x);
				return r;
			}
#ifdef GECOM_USE_X64
			// native x64 bitscan
			inline unsigned bitScanForwardImpl(unsigned long long x) {
				unsigned long r;
				_BitScanForward64(&r, x);
				return r;
			}
#else
			// 64-bit bitscan in terms of x86 bitscan
			inline unsigned bitScanForwardImpl(unsigned long long x) {
				static constexpr unsigned long ulong_bits = CHAR_BIT * sizeof(unsigned long);
				unsigned long x0 = x;
				unsigned long x1 = x >> ulong_bits;
				unsigned r0 = bitScanForwardImpl(x0);
				unsigned r1 = bitScanForwardImpl(x1);
				return x0 ? r0 : r1 + ulong_bits;
			}
#endif
			inline unsigned bitScanForwardImpl(unsigned char x) {
				return bitScanForwardImpl((unsigned long) x);
			}

			inline unsigned bitScanForwardImpl(unsigned short x) {
				return bitScanForwardImpl((unsigned long) x);
			}

			inline unsigned bitScanForwardImpl(unsigned int x) {
				return bitScanForwardImpl((unsigned long) x);
			}
#endif
			// TODO gcc bitscan intrinsics
#endif
		}

		// find index of lowest set bit; undefined if input is 0
		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		inline unsigned bitScanForward(IntT x) {
			using UIntT = std::make_unsigned_t<IntT>;
			return detail::bitScanForwardImpl(reinterpret_cast<UIntT &>(x));
		}

		namespace detail {

			template <typename UIntT, typename = std::enable_if_t<std::is_integral<UIntT>::value && std::is_unsigned<UIntT>::value>>
			inline unsigned bitScanReverseImpl(UIntT x) {
				unsigned i = 0;
				for (; x >>= 1; ++i);
				return i;
			}

#if defined(_MSC_VER)
#ifdef GECOM_USE_X86
			// native x86 bitscan
			inline unsigned bitScanReverseImpl(unsigned long x) {
				unsigned long r;
				_BitScanReverse(&r, x);
				return r;
			}
#ifdef GECOM_USE_X64
			// native x64 bitscan
			inline unsigned bitScanReverseImpl(unsigned long long x) {
				unsigned long r;
				_BitScanReverse64(&r, x);
				return r;
			}
#else
			// 64-bit bitscan in terms of x86 bitscan
			inline unsigned bitScanReverseImpl(unsigned long long x) {
				static constexpr unsigned long ulong_bits = CHAR_BIT * sizeof(unsigned long);
				unsigned long x0 = x;
				unsigned long x1 = x >> ulong_bits;
				unsigned r0 = bitScanReverseImpl(x0);
				unsigned r1 = bitScanReverseImpl(x1);
				return x1 ? r1 + ulong_bits : r0;
			}
#endif
			inline unsigned bitScanReverseImpl(unsigned char x) {
				return bitScanReverseImpl((unsigned long) x);
			}

			inline unsigned bitScanReverseImpl(unsigned short x) {
				return bitScanReverseImpl((unsigned long) x);
			}

			inline unsigned bitScanReverseImpl(unsigned int x) {
				return bitScanReverseImpl((unsigned long) x);
			}
#endif
			// TODO gcc bitscan intrinsics
#endif
		}

		// find index of highest set bit; undefined if input is 0
		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		inline unsigned bitScanReverse(IntT x) {
			using UIntT = std::make_unsigned_t<IntT>;
			return detail::bitScanReverseImpl(reinterpret_cast<UIntT &>(x));
		}

	}

}

#endif // GECOM_INTRINSICS_HPP
