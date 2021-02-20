#pragma once

#include <limits>

#include <sserialize/utility/constants.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/checks.h>
#include <sserialize/algorithm/utilmath.h>

namespace sserialize {
	
class UByteArrayAdapter;

class Size final {
public:
	using underlying_type = uint64_t;
public:
	Size() {}
	Size(Size const &) = default;
	Size(Size &&) = default;
	Size(underlying_type v) : m_v(v) {}
	Size(UByteArrayAdapter const & src) : m_v(src.getOffset(0)) {}
	Size(UByteArrayAdapter & src, UByteArrayAdapter::ConsumeTag) : m_v(src.getOffset()) {}
	Size(UByteArrayAdapter const & src, UByteArrayAdapter::NoConsumeTag) : Size(src) {}
	~Size() {}
	inline Size & operator=(underlying_type v) {
		m_v = v;
		return *this;
	}
	Size & operator=(Size const&) = default;
	Size & operator=(Size &&) = default;
public:
	#define OP(__W) inline bool operator __W(Size const & o) const { return m_v __W o.m_v; }
	OP(!=)
	OP(==)
	OP(<)
	OP(<=)
	OP(>)
	OP(>=)
	#undef OP
public:
#define OP(__W) \
	inline Size & operator __W ## =(Size const & o) { \
		m_v __W ## = o.m_v; \
		return *this; \
	} \
	inline Size operator __W (Size const & o) const { return Size(m_v __W o.m_v); }
	
	OP(*)
	OP(+)
	OP(/)
	OP(-)
#undef OP
public:
	friend inline UByteArrayAdapter & operator<<(UByteArrayAdapter & dest, Size const & v) {
		dest.putOffset(v.m_v);
		return dest;
	}
	friend inline UByteArrayAdapter & operator>>(UByteArrayAdapter & src, Size & v) {
		v.m_v = src.getOffset();
		return src;
	}
public:
	inline operator underlying_type() const { return m_v; }
private:
	underlying_type m_v{0};
};

template<>
struct SerializationInfo<Size> {
	using type = Size;
	static const bool is_fixed_length = true;
	static const OffsetType length = SSERIALIZE_OFFSET_BYTE_COUNT;
	static const OffsetType max_length = length;
	static const OffsetType min_length = length;
	static OffsetType sizeInBytes(const type &) {
		return length;
	}
};

template<typename I, typename J>
inline
typename std::enable_if<std::is_same<Size, I>::value && std::is_unsigned<J>::value, I>::type
narrow_check(J value) {
	if (UNLIKELY_BRANCH(value > sserialize::createMask64(SerializationInfo<Size>::length*8))) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

}