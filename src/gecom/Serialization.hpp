
#ifndef GECOM_SERIALIZATION_HPP
#define GECOM_SERIALIZATION_HPP

// TODO add complete container template params to operators

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

#ifdef _MSC_VER
// byteswap intrinsics
#include <stdlib.h>
#endif

namespace gecom {

	using serialized_size_t = uint64_t;

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

	namespace detail {

		template <typename UIntT, typename = std::enable_if_t<std::is_integral<UIntT>::value && std::is_unsigned<UIntT>::value>>
		inline UIntT byteswap_impl(UIntT x) {
			static constexpr UIntT mask = (UIntT(1) << CHAR_BIT) - 1;
			UIntT r = 0;
			for (int i = 0; i < sizeof(UIntT); ++i) {
				r <<= CHAR_BIT;
				r |= x & mask;
				x >>= CHAR_BIT;
			}
			return r;
		}

#if defined(_MSC_VER)
		inline unsigned short byteswap_impl(unsigned short x) {
			return _byteswap_ushort(x);
		}

		inline unsigned long byteswap_impl(unsigned long x) {
			return _byteswap_ulong(x);
		}

		inline unsigned long long byteswap_impl(unsigned long long x) {
			return _byteswap_uint64(x);
		}
#elif defined(__GNUC__)
		inline uint16_t byteswap_impl(uint16_t x) {
			return __builtin_bswap16(x);
		}

		inline uint32_t byteswap_impl(uint32_t x) {
			return __builtin_bswap32(x);
		}

		inline uint64_t byteswap_impl(uint64_t x) {
			return __builtin_bswap64(x);
		}
#endif

	}

	template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
	inline IntT byteswap(IntT x) {
		using UIntT = std::make_unsigned_t<IntT>;
		UIntT r = detail::byteswap_impl(reinterpret_cast<UIntT &>(x));
		return reinterpret_cast<IntT &>(r);
	}

	class serializer_base {
	public:
		using iostate = std::ios_base::iostate;
		static constexpr iostate goodbit = 0;
		static constexpr iostate badbit = std::ios_base::badbit;
		static constexpr iostate failbit = std::ios_base::failbit;
		static constexpr iostate eofbit = std::ios_base::eofbit;

		using openmode = std::ios_base::openmode;
		static constexpr openmode app = std::ios_base::app;
		static constexpr openmode binary = std::ios_base::binary;
		static constexpr openmode in = std::ios_base::in;
		static constexpr openmode out = std::ios_base::out;
		static constexpr openmode trunc = std::ios_base::trunc;
		static constexpr openmode ate = std::ios_base::ate;

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
			other.m_state = badbit;
		}

		serializer_base & operator=(serializer_base &&other) {
			m_buf = other.m_buf;
			m_endianness = other.m_endianness;
			m_state = other.m_state;
			other.m_buf = nullptr;
			other.m_state = badbit;
			return *this;
		}

		std::streambuf * rdbuf() const {
			return m_buf;
		}

		std::streambuf * rdbuf(std::streambuf *buf) {
			std::streambuf *r = m_buf;
			m_buf = buf;
			clear(buf ? goodbit : badbit);
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

		virtual ~serializer_base() { }
	};

	class serializer : public serializer_base {
	public:
		using serializer_base::serializer_base;

		serializer() { }

		serializer(serializer &&) = default;
		serializer & operator=(serializer &&) = default;

		void flush() {
			m_buf->pubsync();
		}

		void write(const char *buf, size_t count) {
			if (!good()) return;
			if (m_buf->sputn(buf, count) < std::streamsize(count)) setstate(badbit);
		}

		void put(char c) {
			write(&c, 1);
		}

		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		void putInt(IntT x) {
			x = m_endianness == cpuEndian() ? x : byteswap(x);
			write(reinterpret_cast<char *>(&x), sizeof(IntT));
		}

		void putFloat(float x) {
			static_assert(sizeof(float) == sizeof(uint32_t), "assumed sizeof(float) == sizeof(uint32_t)");
			uint32_t x2 = reinterpret_cast<uint32_t &>(x);
			x2 = m_endianness == fpuEndian() ? x2 : byteswap(x2);
			write(reinterpret_cast<char *>(&x2), sizeof(uint32_t));
		}

		void putDouble(double x) {
			static_assert(sizeof(double) == sizeof(uint64_t), "assumed sizeof(double) == sizeof(uint64_t)");
			uint64_t x2 = reinterpret_cast<uint64_t &>(x);
			x2 = m_endianness == fpuEndian() ? x2 : byteswap(x2);
			write(reinterpret_cast<char *>(&x2), sizeof(uint64_t));
		}
		
		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		serializer & operator<<(IntT x) {
			putInt(x);
			return *this;
		}

		serializer & operator<<(float x) {
			putFloat(x);
			return *this;
		}

		serializer & operator<<(double x) {
			putDouble(x);
			return *this;
		}

		serializer & operator<<(endian e) {
			put(e == endian::little ? 0 : 1);
			return *this;
		}

		serializer & operator<<(std::streambuf *sb) {
			if (!good()) return *this;
			std::ostream o(rdbuf());
			o << sb;
			if (!o) setstate(badbit);
			return *this;
		}

		serializer & operator<<(serializer_base & (*func)(serializer_base &)) {
			func(*this);
			return *this;
		}

		serializer & operator<<(serializer & (*func)(serializer &)) {
			func(*this);
			return *this;
		}
	};

	inline serializer & operator<<(serializer &out, const std::string &s) {
		out << serialized_size_t(s.size());
		out.write(s.c_str(), s.size());
		return out;
	}

	template <typename T0, typename T1>
	inline serializer & operator<<(serializer &out, const std::pair<T0, T1> &p) {
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
	inline serializer & operator<<(serializer &out, const std::tuple<TR...> t) {
		detail::serialize_tuple_impl<sizeof...(TR)>::go(out, t);
		return out;
	}

	template <typename T, size_t Size>
	inline serializer & operator<<(serializer &out, const std::array<T, Size> &a) {
		for (const auto &x : a) {
			out << x;
		}
		return out;
	}

	template <size_t Size>
	inline serializer & operator<<(serializer &out, const std::bitset<Size> &s) {
		// TODO serialize bitset
		return out;
	}

	template <typename T>
	inline serializer & operator<<(serializer &out, const std::vector<T> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	inline serializer & operator<<(serializer &out, const std::list<T> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	inline serializer & operator<<(serializer &out, const std::deque<T> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	inline serializer & operator<<(serializer &out, const std::set<T> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename T>
	inline serializer & operator<<(serializer &out, const std::unordered_set<T> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename KeyT, typename ValueT>
	inline serializer & operator<<(serializer &out, const std::map<KeyT, ValueT> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	template <typename KeyT, typename ValueT>
	inline serializer & operator<<(serializer &out, const std::unordered_map<KeyT, ValueT> &v) {
		out << serialized_size_t(v.size());
		for (const auto &x : v) {
			out << x;
		}
		return out;
	}

	class deserializer : public serializer_base {
	public:
		using serializer_base::serializer_base;

		deserializer() { }

		deserializer(deserializer &&) = default;
		deserializer & operator=(deserializer &&) = default;

		traits_type::int_type get() {
			if (!good()) return traits_type::eof();
			auto r = m_buf->sbumpc();
			if (r == traits_type::eof()) setstate(failbit | eofbit);
			return r;
		}

		traits_type::int_type peek() {
			if (!good()) return traits_type::eof();
			return m_buf->sgetc();
		}

		void read(char *buf, size_t count) {
			if (!good()) return;
			size_t rc = m_buf->sgetn(buf, count);
			if (rc < count) setstate(failbit | eofbit);
		}

		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		IntT getInt() {
			IntT x2 = 0;
			read(reinterpret_cast<char *>(&x2), sizeof(IntT));
			return m_endianness == cpuEndian() ? x2 : byteswap(x2);
		}

		float getFloat() {
			static_assert(sizeof(float) == sizeof(uint32_t), "assumed sizeof(float) == sizeof(uint32_t)");
			uint32_t x2 = 0;
			read(reinterpret_cast<char *>(&x2), sizeof(uint32_t));
			x2 = m_endianness == fpuEndian() ? x2 : byteswap(x2);
			return reinterpret_cast<float &>(x2);
		}

		double getDouble() {
			static_assert(sizeof(double) == sizeof(uint64_t), "assumed sizeof(double) == sizeof(uint64_t)");
			uint64_t x2 = 0;
			read(reinterpret_cast<char *>(&x2), sizeof(uint64_t));
			x2 = m_endianness == fpuEndian() ? x2 : byteswap(x2);
			return reinterpret_cast<double &>(x2);
		}

		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		deserializer & operator>>(IntT &x) {
			x = getInt<IntT>();
			return *this;
		}

		deserializer & operator>>(float &x) {
			x = getFloat();
			return *this;
		}

		deserializer & operator>>(double &x) {
			x = getDouble();
			return *this;
		}

		deserializer & operator>>(endian &e) {
			e = get() ? endian::big : endian::little;
			return *this;
		}

		deserializer & operator>>(std::streambuf *sb) {
			if (!good()) return *this;
			std::istream i(rdbuf());
			i >> sb;
			if (i.eof()) setstate(eofbit);
			if (i.fail()) setstate(badbit);
			return *this;
		}

		deserializer & operator<<(serializer_base & (*func)(serializer_base &)) {
			func(*this);
			return *this;
		}

		deserializer & operator<<(deserializer & (*func)(deserializer &)) {
			func(*this);
			return *this;
		}
	};

	inline deserializer & operator>>(deserializer &in, std::string &s) {
		serialized_size_t size = 0;
		in >> size;
		s.assign(size, '\0');
		if (size) in.read(&s[0], size);
		return in;
	}

	template <typename T0, typename T1>
	inline deserializer & operator>>(deserializer &in, std::pair<T0, T1> &p) {
		return in >> p.first >> p.second;
	}

	namespace detail {

		template <size_t Remaining>
		struct deserialize_tuple_impl {
			template <typename TupleT>
			static void go(deserializer &in, TupleT &tup) {
				in >> std::get<(std::tuple_size<TupleT>::value - Remaining)>(tup);
				deserialize_tuple_impl<Remaining - 1>(out, tup);
			}
		};

		template <>
		struct deserialize_tuple_impl<0> {
			template <typename TupleT>
			static void go(deserializer &, TupleT &) { }
		};

	}

	template <typename ...TR>
	inline deserializer & operator>>(deserializer &in, std::tuple<TR...> &t) {
		detail::deserialize_tuple_impl<sizeof...(TR)>::go(in, t);
		return in;
	}

	template <typename T, size_t Size>
	inline deserializer & operator>>(deserializer &in, std::array<T, Size> &a) {
		for (auto &x : a) {
			in >> x;
		}
		return in;
	}

	template <size_t Size>
	inline deserializer & operator>>(deserializer &in, std::bitset<Size> &s) {
		// TODO deserialize bitset
		return in;
	}

	template <typename T>
	inline deserializer & operator>>(deserializer &in, std::vector<T> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.assign(size, T());
		for (auto &x : v) {
			in >> x;
		}
		return in;
	}

	template <typename T>
	inline deserializer & operator>>(deserializer &in, std::list<T> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.assign(size, T());
		for (auto &x : v) {
			in >> x;
		}
		return in;
	}

	template <typename T>
	inline deserializer & operator>>(deserializer &in, std::deque<T> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.assign(size, T());
		for (auto &x : v) {
			in >> x;
		}
		return in;
	}

	template <typename T>
	inline deserializer & operator>>(deserializer &in, std::set<T> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.clear();
		for (serialized_size_t i = 0; i < size; ++i) {
			T x;
			in >> x;
			v.insert(std::move(x));
		}
	}

	template <typename T>
	inline deserializer & operator>>(deserializer &in, std::unordered_set<T> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.clear();
		for (serialized_size_t i = 0; i < size; ++i) {
			T x;
			in >> x;
			v.insert(std::move(x));
		}
	}

	template <typename KeyT, typename ValueT>
	inline deserializer & operator>>(deserializer &in, std::map<KeyT, ValueT> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.clear();
		for (serialized_size_t i = 0; i < size; ++i) {
			std::pair<KeyT, ValueT> x;
			in >> x;
			v.insert(std::move(x));
		}
		return in;
	}

	template <typename KeyT, typename ValueT>
	inline deserializer & operator>>(deserializer &in, std::unordered_map<KeyT, ValueT> &v) {
		serialized_size_t size = 0;
		in >> size;
		v.clear();
		for (serialized_size_t i = 0; i < size; ++i) {
			std::pair<KeyT, ValueT> x;
			in >> x;
			v.insert(std::move(x));
		}
		return in;
	}

	class string_serializer : public serializer {
	private:
		mutable std::stringbuf m_stringbuf;

	public:
		explicit string_serializer(openmode mode_ = out) : m_stringbuf(mode_ | out | binary) {
			serializer::rdbuf(&m_stringbuf);
		}

		explicit string_serializer(const std::string &str_, openmode mode_ = out) : m_stringbuf(str_, mode_ | out | binary) {
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

	class string_deserializer : public deserializer {
	private:
		mutable std::stringbuf m_stringbuf;

	public:
		explicit string_deserializer(openmode mode_ = in) : m_stringbuf(mode_ | in | binary) {
			deserializer::rdbuf(&m_stringbuf);
		}

		explicit string_deserializer(const std::string &str_, openmode mode_ = in) : m_stringbuf(str_, mode_ | in | binary) {
			deserializer::rdbuf(&m_stringbuf);
		}

		string_deserializer(string_deserializer &&other) : m_stringbuf(std::move(other.m_stringbuf)) {
			deserializer::rdbuf(&m_stringbuf);
			serializer_base::swap(other);
		}

		string_deserializer & operator=(string_deserializer &&other) {
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
	private:
		mutable std::filebuf m_filebuf;

	public:
		file_serializer() { }

		explicit file_serializer(const char *path_, openmode mode_ = out) {
			m_filebuf.open(path_, mode_ | out | binary);
			serializer::rdbuf(&m_filebuf);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		explicit file_serializer(const std::string &path_, openmode mode_ = out) {
			m_filebuf.open(path_, mode_ | out | binary);
			serializer::rdbuf(&m_filebuf);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		file_serializer(file_serializer &&other) : m_filebuf(std::move(other.m_filebuf)) {
			serializer::rdbuf(&m_filebuf);
			serializer_base::swap(other);
		}

		file_serializer & operator=(file_serializer &&other) {
			m_filebuf = std::move(other.m_filebuf);
			serializer_base::swap(other);
			return *this;
		}

		std::filebuf * rdbuf() const {
			return &m_filebuf;
		}

		bool is_open() const {
			return m_filebuf.is_open();
		}

		void open(const char *path, openmode mode = out) {
			m_filebuf.open(path, mode | out | binary);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		void open(const std::string &path, openmode mode = out) {
			m_filebuf.open(path, mode | out | binary);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		void close() {
			m_filebuf.close();
		}
	};

	class file_deserializer : public deserializer {
	private:
		mutable std::filebuf m_filebuf;

	public:
		file_deserializer() { }

		explicit file_deserializer(const char *path_, openmode mode_ = in) {
			m_filebuf.open(path_, mode_ | in | binary);
			deserializer::rdbuf(&m_filebuf);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		explicit file_deserializer(const std::string &path_, openmode mode_ = in) {
			m_filebuf.open(path_, mode_ | in | binary);
			deserializer::rdbuf(&m_filebuf);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		file_deserializer(file_deserializer &&other) : m_filebuf(std::move(other.m_filebuf)) {
			deserializer::rdbuf(&m_filebuf);
			serializer_base::swap(other);
		}

		file_deserializer & operator=(file_deserializer &&other) {
			m_filebuf = std::move(other.m_filebuf);
			serializer_base::swap(other);
			return *this;
		}

		std::filebuf * rdbuf() const {
			return &m_filebuf;
		}

		bool is_open() const {
			return m_filebuf.is_open();
		}

		void open(const char *path, openmode mode = out) {
			m_filebuf.open(path, mode | in | binary);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		void open(const std::string &path, openmode mode = out) {
			m_filebuf.open(path, mode | in | binary);
			clear(m_filebuf.is_open() ? goodbit : failbit);
		}

		void close() {
			m_filebuf.close();
		}
	};
}

#endif // GECOM_SERIALIZATION_HPP
