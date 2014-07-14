#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H
/* uint*_t */
#include <stdint.h>
/* memcpy */
#include <string.h>

#include <type_traits>
#include <functional>
#include <limits>
#include <sserialize/utility/utilmath.h>

/* make sure be32toh and be64toh are present */
#if defined(__linux__)
#  if defined(__ANDROID__)
#    include <sys/endian.h>
#    define be16toh(x) betoh16(x)
#    define be32toh(x) betoh32(x)
#    define be64toh(x) betoh64(x)
#    define le16toh(x) letoh16(x)
#    define le32toh(x) letoh32(x)
#    define le64toh(x) letoh64(x)
#  else
#    include <endian.h>
#  endif
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#else
/* htons/htonl */
#include <arpa/inet.h>
#  define htobe16(x) htons(x)
#  define be16toh(x) htons(x)
#  define htobe32(x) htonl(x)
#  define be32toh(x) htonl(x)
#  define htobe64(x) my_htobe64(x)
#  define be64toh(x) htobe64(x)
static uint64_t my_htobe64(uint64_t x) {
	if (htobe32(1) == 1) return x;
	return ((uint64_t)htobe32(x)) << 32 | htobe32(x >> 32);
}

#endif

///Pack functions:
///Current default encoding: little-endian encoding, signed values as two-complement TODO:check for architectures not using two complement

namespace sserialize {

//unpack functions

inline uint8_t up_u8(const uint8_t * src ) {
	return *src;
}

inline uint16_t up_u16(const uint8_t * src ) {
	uint16_t i;
	memcpy(&i, src, sizeof(i));
	return le16toh(i);
}

inline uint32_t up_u32(const uint8_t * src ) {
	uint32_t i;
	memcpy(&i, src, sizeof(i));
	return le32toh(i);
}

inline uint64_t up_u64(const uint8_t * src) {
	uint64_t i;
	memcpy(&i, src, sizeof(i));
	return le64toh(i);
}

inline uint32_t up_u24(const uint8_t * src ) {
	uint32_t i = 0;
	memcpy(&i, src, 3);
	return le32toh(i);
}

inline uint64_t up_u40(const uint8_t * src) {
	uint64_t i = 0;
	memcpy(&i, src, 5);
	return le64toh(i);
}

inline int8_t up_s8(const uint8_t * src) {
	return *src;
}

inline int16_t up_s16(const uint8_t * src) {
	return up_u16(src);
}

inline int32_t up_s24(const uint8_t * src) {
	uint32_t tmp = up_u24(src);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline int32_t up_s32(const uint8_t * src) {
	return up_u32(src);
}

inline int64_t up_s40(const uint8_t * src) {
	uint64_t tmp = up_u40(src);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline int64_t up_s64(const uint8_t * src) {
	return up_u64(src);
}

inline double unpack_double_from_uint64_t(const uint64_t src) {
	union {
		double d;
		uint64_t ui;
	} tmp;
	tmp.ui = src;
	return tmp.d;
}

//BUG: do this right
inline float unpack_float_from_uint32_t(const uint32_t src) {
	union {
		float d;
		uint32_t ui;
	} tmp;
	tmp.ui = src;
	return tmp.d;
}

//pack functions

inline void p_u8(uint8_t src, uint8_t * dest) {
	dest[0] = src;
}

inline void p_u16(uint16_t src, uint8_t * dest) {
	src  = htole16(src);
	memcpy(dest, &src, sizeof(src));
}

inline void p_u32(uint32_t src, unsigned char * dest) {
	src = htole32(src);
	memcpy(dest, &src, sizeof(src));
}

inline void p_u64(uint64_t src, unsigned char * dest) {
	src = htole64(src);
	memcpy(dest, &src, sizeof(src));
}

inline void p_u24(uint32_t src, uint8_t * dest) {
	uint8_t tmp[4];
	p_u32(src, tmp);
	memcpy(dest, tmp, 3);
}

inline void p_u40(uint64_t src, unsigned char * dest) {
	uint8_t tmp[8];
	p_u64(src, tmp);
	memcpy(dest, tmp, 5);
}

inline void p_s8(int8_t src, uint8_t * d) {
	*d = src;
}

inline void p_s16(int16_t src, uint8_t * d) {
	p_u16(src, d);
}

inline void p_s24(int32_t src, uint8_t * d) {
	p_u24( (src < 0 ? ((static_cast<uint32_t>(-src) << 1) | 0x1) : static_cast<uint32_t>(src) << 1 ), d);
}

inline void p_s32(int32_t s, uint8_t * d) {
	p_u32(s, d);
}

inline void p_s40(int64_t s, uint8_t * d) {
	p_u40( (s < 0 ? ((static_cast<uint64_t>(-s) << 1) | 0x1) : static_cast<uint64_t>(s) << 1 ), d);
}

inline void p_s64(int64_t s, uint8_t * d) {
	p_u64(s, d);
}


//BUG: do this right
inline uint64_t pack_double_to_uint64_t(const double src) {
	union {
		double d;
		uint64_t ui;
	} tmp;
	tmp.d = src;
	return tmp.ui;
}

//BUG: do this right
inline uint32_t pack_float_to_uint32_t(const float src) {
	union {
		float d;
		uint32_t ui;
	} tmp;
	tmp.d = src;
	return tmp.ui;
}

typedef void(*PackFunctionsFuncPtr)(uint32_t s, uint8_t * d);
typedef int(*VlPackUint32FunctionsFuncPtr)(uint32_t s, uint8_t * d);
typedef int(*VlPackInt32FunctionsFuncPtr)(int32_t s, uint8_t * d);

typedef uint32_t(*VlUnPackUint32FunctionsFuncPtr)(uint8_t * s, int * len);
typedef int32_t(*VlUnPackInt32FunctionsFuncPtr)(int8_t * s, int * len);

template<typename UnsignedType>
int p_v(typename std::enable_if<std::is_unsigned<UnsignedType>::value && std::is_integral<UnsignedType>::value, UnsignedType >::type s, uint8_t * d) {
	int8_t i = 0;
	do {
		d[i] = s & 0x7F;
		s = s >> 7;
		if (s)
			d[i] |= 0x80;
		++i;
	} while (s);
	return i;
}

template<typename SignedType>
int p_v(typename std::enable_if<std::is_signed<SignedType>::value && std::is_integral<SignedType>::value, SignedType >::type s, uint8_t * d) {
	typedef typename std::make_unsigned<SignedType>::type UnsignedType;
	UnsignedType tmp;
	if (s < 0) {
		tmp = -s;
		tmp <<= 1;
		tmp |= 0x1;
	}
	else {
		tmp = s;
		tmp <<= 1;
	}
	return p_v<UnsignedType>(tmp, d);
}


inline int p_vu32(uint32_t s, uint8_t * d) { return p_v<uint32_t>(s, d);}

inline int p_vu32pad4(uint32_t s, uint8_t * d) {
	int8_t i = 0;
	while (i < 4) {
		d[i] = (s & 0x7F) | 0x80;
		s = s >> 7;
		++i;
	}
	d[3] = d[3] & 0x7F;
	return i;
}

inline int p_vu64(uint64_t s, uint8_t * d) { return p_v<uint64_t>(s, d);}

inline int p_vs32(int32_t s, uint8_t * d) { return p_v<int32_t>(s, d);}

inline int p_vs32pad4(int32_t s, uint8_t * d) {
	uint32_t tmp;
	if (s < 0) {
		tmp = -s;
		tmp = (tmp << 1) | 0x1;
	}
	else {
		tmp = s;
		tmp = (tmp << 1);
	}
	return p_vu32pad4(tmp, d);
}

inline int p_vs64(int64_t s, uint8_t * d) { return p_v<int64_t>(s, d);}

template<typename UnsignedType>
typename std::enable_if<std::is_unsigned<UnsignedType>::value && std::is_integral<UnsignedType>::value, UnsignedType >::type up_v(uint8_t * s, int * len) {
	const int maxLen = std::numeric_limits<UnsignedType>::digits/7 + (std::numeric_limits<uint32_t>::digits % 7 ? 1 : 0);
	UnsignedType retVal = 0;
	int myLen = 0;
	int myShift = 0;
	do {
		retVal |= static_cast<UnsignedType>(s[myLen] & 0x7F) << myShift;
		++myLen;
		myShift += 7;
	}
	while (myLen < maxLen && s[myLen-1] & 0x80);
	
	if (len)
		*len = myLen;
		
	return retVal;
}

template<typename SignedType>
typename std::enable_if<std::is_signed<SignedType>::value && std::is_integral<SignedType>::value, SignedType >::type up_v(uint8_t * s, int * len) {
	typedef typename std::make_unsigned<SignedType>::type UnsignedType;
	UnsignedType tmp = up_v<UnsignedType>(s, len);
	//check if signed or unsigned
	SignedType stmp = (tmp >> 1);
	if (tmp & 0x1)
		return -stmp;
	return stmp;
}

inline uint32_t up_vu32(uint8_t * s, int * len) { return up_v<uint32_t>(s, len);}
inline uint64_t up_vu64(uint8_t * s, int * len) { return up_v<uint64_t>(s, len);}
inline int32_t up_vs32(uint8_t * s, int * len) { return up_v<int32_t>(s, len);}
inline int64_t up_vs64(uint8_t* s, int* len) { return up_v<int64_t>(s, len);}

template<typename UnsignedType>
int psize_v(typename std::enable_if<std::is_unsigned<UnsignedType>::value && std::is_integral<UnsignedType>::value, UnsignedType >::type s) {
	int8_t i = 0;
	do {
		s >>= 7;
		++i;
	} while (s);
	return i;
}

template<typename SignedType>
int psize_v(typename std::enable_if<std::is_signed<SignedType>::value && std::is_integral<SignedType>::value, SignedType >::type s) {
	typedef typename std::make_unsigned<SignedType>::type UnsignedType;
	UnsignedType tmp;
	if (s < 0) {
		tmp = -s;
		tmp <<= 1;
		tmp |= 0x1;
	}
	else {
		tmp = s;
		tmp <<= 1;
	}
	return psize_v<UnsignedType>(tmp);
}

inline int psize_vu32(uint32_t s) {
	return psize_v<uint32_t>(s);
}

inline int psize_vs32(int32_t s) {
	return psize_v<int32_t>(s);
}

inline int psize_vu64(uint64_t s) {
	return psize_v<uint64_t>(s);
}

inline int psize_vs64(int64_t s) {
	return psize_v<int64_t>(s);
}

}//end namespace

#endif
