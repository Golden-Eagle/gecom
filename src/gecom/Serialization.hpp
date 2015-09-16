
#ifndef GECOM_SERIALIZATION_HPP
#define GECOM_SERIALIZATION_HPP

#include <cstdint>
#include <climits>
#include <iostream>
#include <type_traits>

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
	private:
		void put_forward(const char *pstart, size_t count) {
			if (m_buf->sputn(pstart, count) < count) setstate(badbit);
		}

		void put_reverse(const char *pstart, size_t count) {
			// TODO optimize
			for (const char *p = pstart + count; p-- > pstart; ) {
				if (m_buf->sputc(*p) == traits_type::eof()) {
					setstate(badbit);
					return;
				}
			}
		}

	public:
		using serializer_base::serializer_base;

		template <typename IntT, typename = std::enable_if_t<std::is_integral<IntT>::value>>
		void putInt(IntT x) {
			if (m_endianness == cpuEndian()) {
				put_forward(reinterpret_cast<char *>(&x), sizeof(IntT));
			} else {
				put_reverse(reinterpret_cast<char *>(&x), sizeof(IntT));
			}
		}

		template <typename FloatT, typename = std::enable_if_t<std::is_floating_point<FloatT>::value>>
		void putFloat(FloatT x) {
			if (m_endianness = fpuEndian()) {
				put_forward(reinterpret_cast<char *>(&x), sizeof(FloatT));
			} else {
				put_reverse(reinterpret_cast<char *>(&x), sizeof(FloatT));
			}
		}

		// TODO
	};

	class deserializer : public serializer_base {
	public:
		using serializer_base::serializer_base;

		// TODO
	};

}

#endif // GECOM_SERIALIZATION_HPP
