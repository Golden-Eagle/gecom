
#ifndef GECOM_SERIALIZATION_HPP
#define GECOM_SERIALIZATION_HPP

#include <cstdint>
#include <climits>
#include <type_traits>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <utility>
#include <tuple>
#include <array>
#include <bitset>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace gecom {

	enum class endian {
		little, big
	};

	inline std::ostream & operator<<(std::ostream &out, endian e) {
		switch (e) {
		case endian::little:
			out << "little-endian";
			break;
		case endian::big:
			out << "big-endian";
			break;
		default:
			out << "?-endian";
			break;
		}
		return out;
	}

	inline endian cpuEndian() {
		static_assert(CHAR_BIT == 8, "assumed CHAR_BIT == 8");
		static const unsigned char c[] { 0x01, 0x00, 0x00, 0x00 };
		static const endian e = (*reinterpret_cast<const uint32_t *>(c) == 1) ? endian::little : endian::big;
		return e;
	}

	inline endian fpuEndian() {
		static_assert(CHAR_BIT == 8, "assumed CHAR_BIT == 8");
		static_assert(sizeof(float) == 4, "assumed sizeof(float) == 4");
		static const unsigned char c[] { 0x01, 0x00, 0x00, 0x80 };
		static const endian e = (*reinterpret_cast<const float *>(c) < 0.f) ? endian::little : endian::big;
		return e;
	}

	template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
	inline IntT flipEndian(IntT x) {
		static constexpr IntT mask = (IntT(1) << CHAR_BIT) - 1;
		IntT r = 0;
		for (int i = 0; i < sizeof(IntT); ++i) {
			r <<= CHAR_BIT;
			r |= x & mask;
			x >>= CHAR_BIT;
		}
		return r;
	}

	class serializer_base {
	public:
		using iostate = std::ios_base::iostate;
		static constexpr iostate goodbit = 0;
		static constexpr iostate badbit = std::ios_base::badbit;
		static constexpr iostate failbit = std::ios_base::failbit;
		static constexpr iostate eofbit = std::ios_base::eofbit;

		using traits_type = std::char_traits<char>;

	protected:
		std::streambuf *m_buf = nullptr;
		endian m_endianness = endian::big;
		iostate m_state = badbit;

		// swap everything except rdbuf
		void swap(serializer_base &other) noexcept {
			using std::swap;
			swap(m_endianness, other.m_endianness);
			swap(m_state, other.m_state);
		}

	public:
		serializer_base() { }

		explicit serializer_base(std::streambuf *buf_) : m_buf(buf_), m_state(goodbit) { }

		serializer_base(const serializer_base &) = delete;
		serializer_base & operator=(const serializer_base &) = delete;

		serializer_base(serializer_base &&other) :
			m_buf(other.m_buf), m_endianness(other.m_endianness), m_state(other.m_state)
		{
			other.m_buf = nullptr;
		}

		serializer_base & operator=(serializer_base &&other) {
			m_buf = other.m_buf;
			m_endianness = other.m_endianness;
			m_state = other.m_state;
			other.m_buf = nullptr;
			return *this;
		}

		std::streambuf * rdbuf() const {
			return m_buf;
		}

		std::streambuf * rdbuf(std::streambuf *buf) {
			std::streambuf *r = m_buf;
			m_buf = buf;
			clear();
			return r;
		}

		endian endianness() const {
			return m_endianness;
		}

		endian endianness(endian e) {
			endian r = m_endianness;
			m_endianness = e;
			return r;
		}

		bool good() const {
			return !m_state;
		}

		bool eof() const {
			return m_state & eofbit;
		}

		bool fail() const {
			return m_state & (failbit | badbit);
		}

		bool bad() const {
			return m_state & badbit;
		}

		bool operator!() const {
			return fail();
		}

		operator bool() const {
			return !fail();
		}

		iostate rdstate() const {
			return m_state;
		}

		void setstate(iostate s) {
			clear(m_state | s);
		}

		void clear(iostate s = goodbit) {
			m_state = s;
		}
	};

	class serializer : public serializer_base {
	public:
		using serializer_base::serializer_base;

		serializer() { }

		serializer(serializer &&) = default;
		serializer & operator=(serializer &&) = default;

		void write(const char *pstart, size_t count) {
			if (fail()) return;
			if (m_buf->sputn(pstart, count) < count) setstate(badbit);
		}

		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		void put(IntT x) {
			x = m_endianness == cpuEndian() ? x : flipEndian(x);
			write(reinterpret_cast<char *>(&x), sizeof(IntT));
		}

		void put(float x) {
			static_assert(sizeof(float) == sizeof(uint32_t), "assumed sizeof(float) == sizeof(uint32_t)");
			uint32_t x2 = reinterpret_cast<uint32_t &>(x);
			x2 = m_endianness == fpuEndian() ? x2 : flipEndian(x2);
			write(reinterpret_cast<char *>(&x2), sizeof(uint32_t));
		}

		void put(double x) {
			static_assert(sizeof(double) == sizeof(uint64_t), "assumed sizeof(double) == sizeof(uint64_t)");
			uint64_t x2 = reinterpret_cast<uint64_t &>(x);
			x2 = m_endianness == fpuEndian() ? x2 : flipEndian(x2);
			write(reinterpret_cast<char *>(&x2), sizeof(uint64_t));
		}
		
		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		serializer & operator<<(IntT x) {
			put(x);
			return *this;
		}

		serializer & operator<<(float x) {
			put(x);
			return *this;
		}

		serializer & operator<<(double x) {
			put(x);
			return *this;
		}

		serializer & operator<<(std::streambuf *sb) {
			std::ostream o(rdbuf());
			o << sb;
			if (!o) setstate(badbit);
			return *this;
		}
	};

	serializer & operator<<(serializer &out, const std::string &s) {
		out << s.size();
		out.write(s.c_str(), s.size());
		return out;
	}

	template <typename T0, typename T1>
	serializer & operator<<(serializer &out, const std::pair<T0, T1> &p) {
		return out << p.first << p.second;
	}

	namespace detail {

		template <size_t Remaining>
		struct serialize_tuple_impl {
			template <typename TupleT>
			static void go(serializer &out, const TupleT &tup) {
				out << std::get<(std::tuple_size<TupleT>::value - Remaining)>(tup);
				serialize_tuple_impl<Remaining - 1>(out, tup);
			}
		};

		template <>
		struct serialize_tuple_impl<0> {
			template <typename TupleT>
			static void go(serializer &, const TupleT &) { }
		};

	}

	template <typename ...TR>
	serializer & operator<<(serializer &out, const std::tuple<TR...> t) {
		detail::serialize_tuple_impl<std::tuple_size<std::tuple<TR...>>::value>::go(out, t);
		return out;
	}

	template <typename T, size_t Size>
	serializer & operator<<(serializer &out, const std::array<T, Size> &a) {
		for (const auto &x : a) {
			out << x;
		}
		return out;
	}

	template <size_t Size>
	serializer & operator<<(serializer &out, const std::bitset<Size> &s) {
		// TODO serialize bitset
		return out;
	}

	template <typename T>
	serializer & operator<<(serializer &out, const std::vector<T> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	serializer & operator<<(serializer &out, const std::list<T> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	serializer & operator<<(serializer &out, const std::deque<T> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	serializer & operator<<(serializer &out, const std::set<T> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	serializer & operator<<(serializer &out, const std::unordered_set<T> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename KeyT, typename ValueT>
	serializer & operator<<(serializer &out, const std::map<KeyT, ValueT> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename KeyT, typename ValueT>
	serializer & operator<<(serializer &out, const std::unordered_map<KeyT, ValueT> &v) {
		out << v.size();
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	class deserializer : public serializer_base {
	public:
		using serializer_base::serializer_base;

		// TODO deserializer
	};

	class string_serializer : public serializer {
	private:
		mutable std::stringbuf m_stringbuf;

	public:
		string_serializer() { }

		string_serializer(const std::string &str) : m_stringbuf(str) {
			serializer::rdbuf(&m_stringbuf);
		}

		string_serializer(string_serializer &&other) : m_stringbuf(std::move(other.m_stringbuf)) {
			serializer::rdbuf(&m_stringbuf);
			serializer_base::swap(other);
		}

		string_serializer & operator=(string_serializer &&other) {
			m_stringbuf = std::move(other.m_stringbuf);
			serializer_base::swap(other);
			return *this;
		}

		std::stringbuf * rdbuf() const {
			return &m_stringbuf;
		}

		std::string str() const {
			return m_stringbuf.str();
		}
	};

	class file_serializer : public serializer {
		// TODO filer serializer
	};

	class string_deserializer : public deserializer {
		// TODO string deserializer
	};

	class file_deserializer : public deserializer {
		// TODO file deserializer
	};
}

#endif // GECOM_SERIALIZATION_HPP
