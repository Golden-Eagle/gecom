/*
 * GECom Miscellaneous Utility Header
 *
 * aka shit that needs to go somewhere
 */

#ifndef GECOM_UTIL_HPP
#define GECOM_UTIL_HPP

#include <cctype>
#include <cstdint>
#include <string>
#include <algorithm>
#include <memory>
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <type_traits>

#include "Terminal.hpp"

#include "Initial3D.hpp"

// this alias will be available by default in new i3d
namespace i3d = initial3d;

namespace gecom {
	
	namespace util {

		// trim leading and trailing whitespace
		inline std::string trim(const std::string &s) {
			auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) { return std::isspace(c); });
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) { return std::isspace(c); }).base();
			return wsback <= wsfront ? std::string() : std::string(wsfront, wsback);
		}

		// check if a string is a valid C-like identifier
		inline bool isIdentifier(const std::string &s) {
			if (s.empty()) return false;
			if (!isalpha(s[0]) && s[0] != '_') return false;
			for (char c : s) {
				if (!isalnum(c) && c != '_') return false;
			}
			return true;
		}

		// unused() base case
		inline void unused() { }

		// function to declare things as unused
		template <typename T1, typename... TR>
		inline void unused(const T1 &t1, const TR &...tr) {
			(void) t1;
			unused(tr...);
		}

		namespace detail {
			template <typename FunT, typename ArgTupleT, size_t ...IR>
			inline decltype(auto) call_impl(FunT &&fun, ArgTupleT &&args, std::index_sequence<IR...>) {
				// when args tuple is empty, it ends up unused
				unused(args);
				return fun(std::get<IR>(args)...);
			}
		}

		// call a function by unpacking arguments from a tuple-like object
		template <typename FunT, typename ArgTupleT>
		inline decltype(auto) call(FunT &&fun, ArgTupleT &&args) {
			return detail::call_impl(
				std::forward<FunT>(fun),
				std::forward<ArgTupleT>(args),
				std::make_index_sequence<std::tuple_size<std::decay_t<ArgTupleT>>::value>()
			);
		}

		// priority queue with a moving pop operation
		template <typename T, typename CompareT = std::less<T>>
		class priority_queue {
		private:
			std::vector<T> m_data;
			CompareT m_compare;

		public:
			explicit priority_queue(const CompareT &compare_ = CompareT()) : m_compare(compare_) { }

			template <typename InputItT>
			priority_queue(InputItT first_, InputItT last_, const CompareT &compare_ = CompareT()) :
				m_data(first_, last_), m_compare(compare_)
			{
				std::make_heap(m_data.begin(), m_data.end(), m_compare);
			}

			priority_queue(priority_queue &&) = default;
			priority_queue & operator=(priority_queue &&) = default;

			bool empty() const noexcept {
				return m_data.empty();
			}

			void clear() noexcept {
				m_data.clear();
			}

			size_t size() const noexcept {
				return m_data.size();
			}

			void push(T t) {
				m_data.emplace_back(std::move(t));
				std::push_heap(m_data.begin(), m_data.end(), m_compare);
			}

			template <typename ...ArgTR>
			void emplace(ArgTR &&...args) {
				m_data.emplace_back(std::forward<ArgTR>(args)...);
				std::push_heap(m_data.begin(), m_data.end(), m_compare);
			}

			const T & top() const {
				return m_data.front();
			}

			T pop() {
				std::pop_heap(m_data.begin(), m_data.end(), m_compare);
				T result = std::move(m_data.back());
				m_data.pop_back();
				return result;
			}
		};

		// hexdump memory to stream
		class hexdump_t {
		private:
			const void *m_data = nullptr;
			size_t m_size = 0;
			bool m_color = false;

			// print bytes as hex
			static void printHex(std::ostream &out, const unsigned char *pdata, ptrdiff_t size, int pad, bool color) {
				char oldfill = out.fill();
				out.fill('0');
				const unsigned char *pend = pdata + size;
				for (; pdata < pend; pdata++, pad--) {
					unsigned c = *pdata;
					out << std::setw(2) << std::hex << c << ' ';
				}
				for (; pad-- > 0; ) {
					out << "   ";
				}
				out.fill(oldfill);
			}

			// print bytes as string, escaping unprintables
			static void printSafe(std::ostream &out, const unsigned char *pdata, ptrdiff_t size, int pad, bool color) {
				const unsigned char *pend = pdata + size;
				for (; pdata < pend; pdata++, pad--) {
					unsigned char c = *pdata;
					if (c < 33 || c > 126) {
						out << '.';
					} else {
						if (color) out << terminal::boldYellow;
						out << c;
						if (color) out << terminal::reset;
					}
				}
				for (; pad-- > 0; ) {
					out << ' ';
				}
			}

		public:
			hexdump_t() { }
			hexdump_t(const void *data_, size_t size_, bool color_ = false) : m_data(data_), m_size(size_), m_color(color_) { }

			inline friend std::ostream & operator<<(std::ostream &out, const hexdump_t &hex) {
				char oldfill = out.fill();
				out.fill('0');
				const unsigned char *pdata = reinterpret_cast<const unsigned char *>(hex.m_data);
				const unsigned char *pend = pdata + hex.m_size;
				uint_least16_t index = 0;
				for (; pdata < pend; pdata += 16, index += 16) {
					if (hex.m_color) out << terminal::boldBlack;
					out << "0x" << std::setw(4) << std::hex << index << " : ";
					if (hex.m_color) out << terminal::reset;
					ptrdiff_t remaining = pend - pdata;
					printHex(out, pdata, std::min<ptrdiff_t>(remaining, 8), 8, hex.m_color);
					out << ' ';
					printHex(out, pdata + 8, std::min<ptrdiff_t>(remaining - 8, 8), 8, hex.m_color);
					out << ": ";
					printSafe(out, pdata, std::min<ptrdiff_t>(remaining, 16), 16, hex.m_color);
					out << std::endl;
				}
				out.fill(oldfill);
				return out;
			}
		};

		inline hexdump_t hexdump(const void *data, size_t size, bool color = false) {
			return hexdump_t(data, size, color);
		}

		inline hexdump_t hexdumpc(const void *data, size_t size) {
			return hexdump_t(data, size, true);
		}

		template <typename T>
		inline hexdump_t hexdumpc(const T &t) {
			return hexdump(t, true);
		}

		inline hexdump_t hexdump(const std::string &s, bool color = false) {
			return hexdump(s.c_str(), s.size(), color);
		}
	}
}

#endif // GECOM_UTIL_HPP
