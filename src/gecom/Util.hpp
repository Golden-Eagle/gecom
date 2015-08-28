/*
 * GECom Miscellaneous Utility Header
 *
 * aka shit that needs to go somewhere
 */

#ifndef GECOM_UTIL_HPP
#define GECOM_UTIL_HPP

#include <cctype>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <utility>
#include <type_traits>

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

		// function to declare things as unused
		template <typename T1, typename... TR>
		inline void unused(const T1 &t1, const TR &...tr) {
			(void) t1;
			unused(tr...);
		}

		// unused() base case
		inline void unused() { }

		namespace detail {
			template <typename FunT, typename ArgTupleT, size_t ...IR>
			inline decltype(auto) call_impl(FunT &&fun, ArgTupleT &&args, std::index_sequence<IR...>) {
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

			bool empty() {
				return m_data.empty();
			}

			size_t size() {
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

	}
}

#endif // GECOM_UTIL_HPP
