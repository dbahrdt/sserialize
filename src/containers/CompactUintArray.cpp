#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/utility/assert.h>
#include <stdint.h>
#include <iostream>

namespace sserialize {

void CompactUintArrayPrivate::calcBegin(const SizeType pos, sserialize::UByteArrayAdapter::OffsetType& posStart, uint8_t & initShift, Bits bpn) const {
	posStart = sserialize::multiplyDiv64(pos, bpn, 8);
	initShift = (pos == 0 ? 0 : narrow_check<uint8_t>(sserialize::multiplyMod64(pos, bpn, 8)));
}

CompactUintArrayPrivate::CompactUintArrayPrivate() : RefCountObject() {}

CompactUintArrayPrivate::CompactUintArrayPrivate(const UByteArrayAdapter& adap) :
RefCountObject(),
m_data(adap)
{}

CompactUintArrayPrivate::~CompactUintArrayPrivate() {}

CompactUintArrayPrivate::Bits CompactUintArrayPrivate::bpn() const {
	return 0;
}

CompactUintArrayPrivateEmpty::CompactUintArrayPrivateEmpty(): CompactUintArrayPrivate() {}

CompactUintArrayPrivateEmpty::~CompactUintArrayPrivateEmpty() {}

CompactUintArrayPrivate::Bits CompactUintArrayPrivateEmpty::bpn() const {
	return 0;
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateEmpty::at(SizeType /*pos*/) const {
	return 0;
}


CompactUintArrayPrivate::value_type CompactUintArrayPrivateEmpty::set(const SizeType /*pos*/, value_type value) {
	return ~value;
}


CompactUintArrayPrivateVarBits::CompactUintArrayPrivateVarBits(const sserialize::UByteArrayAdapter& adap, Bits bpn) :
CompactUintArrayPrivate(adap),
m_bpn(bpn)
{
	if (m_bpn == 0) {
		throw sserialize::TypeOverflowException("bpn=" + std::to_string(bpn) + "<1");
	}
	else if (m_bpn > 32) {
		throw sserialize::TypeOverflowException("bpn=" + std::to_string(bpn) + ">32");
	}
	m_mask = createMask(m_bpn);
}

CompactUintArrayPrivateVarBits::~CompactUintArrayPrivateVarBits() {}

CompactUintArrayPrivateVarBits::Bits CompactUintArrayPrivateVarBits::bpn() const {
	return m_bpn;
}

CompactUintArrayPrivateVarBits::value_type CompactUintArrayPrivateVarBits::at(const SizeType pos) const {
	UByteArrayAdapter::OffsetType posStart;
	uint8_t initShift;
	calcBegin(pos, posStart, initShift, m_bpn);
	uint32_t res = static_cast<uint32_t>(m_data.at(posStart)) >> initShift;

	uint32_t byteShift = 8 - initShift;
	posStart++;
	for(uint32_t bitsRead = 8 - initShift, i  = 0; bitsRead < m_bpn; bitsRead+=8, i++) {
		uint32_t newByte = m_data.at(posStart+i);
		newByte = (newByte << (8*i+byteShift));
		res |= newByte;
	}
	return (res & m_mask);
}


//Idee: zunächst müssen alle betroffenen bits genullt werden.
//anschließend den neuen wert mit einem ODER setzen
CompactUintArrayPrivateVarBits::value_type CompactUintArrayPrivateVarBits::set(const SizeType pos, value_type value) {
	value = value & m_mask;
	
	UByteArrayAdapter::OffsetType startPos;
	uint8_t initShift;
	calcBegin(pos, startPos, initShift, m_bpn);

	if (initShift+m_bpn <= 8) { //everything is within the first
		uint8_t mask = (uint8_t)createMask(initShift); //sets the lower bits
		mask = (uint8_t)(mask | (~createMask(initShift+m_bpn))); //sets the higher bits
		uint8_t & d = m_data[startPos];
		d = (uint8_t) (d & mask);
		d = (uint8_t)(d | (value << initShift));
	}
	else { //we have to set multiple bytes
		uint32_t bitsSet = 0;
		uint8_t mask = (uint8_t)createMask((uint8_t)initShift); //sets the lower bits
		uint8_t & d = m_data[startPos];
		d = (uint8_t)(d & mask);
		d = (uint8_t)(d | ((value << initShift) & 0xFF));
		bitsSet += 8-initShift;
		uint8_t lastByteBitCount = (uint8_t)( (m_bpn+initShift) % 8 );

		//Now set all the midd bytes:
		uint32_t curShift = bitsSet;
		uint32_t count = 1;
		for(; curShift+lastByteBitCount < m_bpn; curShift+=8, count++) {
			m_data[startPos+count] = (uint8_t)( (value >> curShift) & 0xFF );
		}

		//Now set the remaining bits
		if (lastByteBitCount > 0) {
			uint8_t & d = m_data[startPos+count];
			d = (uint8_t)( d & ( ~createMask(lastByteBitCount) ) );
			d = (uint8_t)(d | ( value >> (m_bpn-lastByteBitCount) ) );
		}

	}
	return value;
}

CompactUintArrayPrivateVarBits64::CompactUintArrayPrivateVarBits64(const UByteArrayAdapter & adap, uint32_t bpn) :
CompactUintArrayPrivate(adap),
m_bpn(bpn)
{
	if (m_bpn == 0) {
		m_bpn = 1;
	}
	else if (m_bpn > 64) {
		m_bpn = 64;
	}
	m_mask = createMask64(m_bpn);
}

CompactUintArrayPrivateVarBits64::~CompactUintArrayPrivateVarBits64() {}

uint32_t CompactUintArrayPrivateVarBits64::bpn() const {
	return m_bpn;
}

uint64_t CompactUintArrayPrivateVarBits64::at(SizeType pos) const {
	UByteArrayAdapter::OffsetType posStart;
	uint8_t initShift;
	calcBegin(pos, posStart, initShift, m_bpn);
	uint64_t res = static_cast<uint64_t>(m_data.at(posStart)) >> initShift;

	uint8_t byteShift = (uint8_t)(8 - initShift);
	posStart++;
	for(uint32_t bitsRead = byteShift, i  = 0; bitsRead < m_bpn; bitsRead+=8, i++) {
		uint64_t newByte = m_data.at(posStart+i);
		newByte = (newByte << (8*i+byteShift));
		res |= newByte;
	}
	return (res & m_mask);
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateVarBits64::set(const SizeType pos, value_type value) {
	value = value & m_mask;
	
	UByteArrayAdapter::OffsetType startPos;
	uint8_t initShift;
	calcBegin(pos, startPos, initShift, m_bpn);

	if (initShift+m_bpn <= 8) { //everything is within the first
		uint8_t mask = (uint8_t)createMask(initShift); //sets the lower bits
		mask = (uint8_t) (mask | ( ~createMask(initShift+m_bpn) ) ); //sets the higher bits
		uint8_t & d = m_data[startPos];
		d = (uint8_t)( d & mask );
		d = (uint8_t)( d | ( value << initShift ) );
	}
	else { //we have to set multiple bytes
		uint32_t bitsSet = 0;
		uint8_t mask = (uint8_t)createMask(initShift); //sets the lower bits
		uint8_t & d = m_data[startPos];
		d = (uint8_t)( d & mask );
		d = (uint8_t)( d | ( (value << initShift) & 0xFF ) );
		bitsSet += 8-initShift;
		uint8_t lastByteBitCount = (uint8_t)( (m_bpn+initShift) % 8 );

		//Now set all the midd bytes:
		uint32_t curShift = bitsSet;
		uint32_t count = 1;
		for(; curShift+lastByteBitCount < m_bpn; curShift+=8, count++) {
			m_data[startPos+count] = (uint8_t)( (value >> curShift) & 0xFF );
		}

		//Now set the remaining bits
		if (lastByteBitCount > 0) {
			uint8_t & d = m_data[startPos+count];
			d = (uint8_t)( d & ( ~createMask(lastByteBitCount) ) );
			d = (uint8_t)( d | ( value >> (m_bpn-lastByteBitCount) ) );
		}

	}
	return value;
}

CompactUintArrayPrivateU8::CompactUintArrayPrivateU8(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

CompactUintArrayPrivateU8::~CompactUintArrayPrivateU8() {}

uint32_t CompactUintArrayPrivateU8::bpn() const{
	return 8;
}


CompactUintArrayPrivate::value_type CompactUintArrayPrivateU8::at(SizeType pos) const {
    return m_data.getUint8(pos);
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateU8::set(const SizeType pos, value_type value) {
	m_data.putUint8(pos, (uint8_t)value);
	return value;
}

CompactUintArrayPrivateU16::CompactUintArrayPrivateU16(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

CompactUintArrayPrivateU16::~CompactUintArrayPrivateU16() {}

uint32_t CompactUintArrayPrivateU16::bpn() const {
	return 16;
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateU16::at(SizeType pos) const {
    return m_data.getUint16( SerializationInfo<uint16_t>::length*pos);
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateU16::set(const SizeType pos, value_type value) {
	m_data.putUint16(SerializationInfo<uint16_t>::length*pos, uint16_t(value));
	return uint16_t(value);
}

CompactUintArrayPrivateU24::CompactUintArrayPrivateU24(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

CompactUintArrayPrivateU24::~CompactUintArrayPrivateU24() {}

uint32_t CompactUintArrayPrivateU24::bpn() const {
	return 24;
}


CompactUintArrayPrivate::value_type CompactUintArrayPrivateU24::at(SizeType pos) const
{
    return m_data.getUint24(static_cast<UByteArrayAdapter::OffsetType>(3)*pos);
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateU24::set(const SizeType pos, value_type value) {
	m_data.putUint24(static_cast<UByteArrayAdapter::OffsetType>(3)*pos, value);
	return value;
}

CompactUintArrayPrivateU32::CompactUintArrayPrivateU32(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

CompactUintArrayPrivateU32::~CompactUintArrayPrivateU32() {}

uint32_t CompactUintArrayPrivateU32::bpn() const {
	return 32;
}


CompactUintArrayPrivate::value_type CompactUintArrayPrivateU32::at(SizeType pos) const {
    return m_data.getUint32(SerializationInfo<uint32_t>::length*pos);
}

CompactUintArrayPrivate::value_type CompactUintArrayPrivateU32::set(const SizeType pos, value_type value) {
    m_data.putUint32(SerializationInfo<uint32_t>::length*pos, value);
	return value;
}

CompactUintArray::CompactUintArray(CompactUintArrayPrivate * priv) :
RCWrapper< sserialize::CompactUintArrayPrivate >(priv)
{}

void CompactUintArray::setPrivate(const sserialize::UByteArrayAdapter& array, uint32_t bitsPerNumber) {
	if (bitsPerNumber == 0)
		bitsPerNumber = 1;
	else if (bitsPerNumber > 64)
		bitsPerNumber = 64;

	m_maxCount = (SizeType) std::min<uint64_t>((static_cast<uint64_t>(array.size())*8)/bitsPerNumber, std::numeric_limits<SizeType>::max());

	switch(bitsPerNumber) {
	case (8):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU8(array));
		break;
	case (16):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU16(array));
		break;
	case (24):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU24(array));
		break;
	case (32):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU32(array));
		break;
	default:
		if (bitsPerNumber <= 32)
			MyBaseClass::setPrivate(new CompactUintArrayPrivateVarBits(array, bitsPerNumber));
		else
			MyBaseClass::setPrivate(new CompactUintArrayPrivateVarBits64(array, bitsPerNumber));
		break;
	}
}

void CompactUintArray::setMaxCount(CompactUintArray::SizeType maxCount) {
	m_maxCount = maxCount;
}

CompactUintArray::CompactUintArray() :
RCWrapper< sserialize::CompactUintArrayPrivate > (new CompactUintArrayPrivateEmpty()),
m_maxCount(0)
{
}

CompactUintArray::CompactUintArray(const UByteArrayAdapter & array, uint32_t bitsPerNumber) :
RCWrapper< sserialize::CompactUintArrayPrivate >(0)
{
	setPrivate(array, bitsPerNumber);
}

CompactUintArray::CompactUintArray(const sserialize::UByteArrayAdapter& array, Bits bitsPerNumber, SizeType max_size) :
RCWrapper< sserialize::CompactUintArrayPrivate >(0)
{
	setPrivate(array, bitsPerNumber);
	if (array.size() < CompactUintArray::minStorageBytes(bitsPerNumber, max_size)) {
		auto wantSize = CompactUintArray::minStorageBytes(bitsPerNumber, max_size);
		throw sserialize::OutOfBoundsException(
				"CompactUintArray(bpn=" + std::to_string(bitsPerNumber) +
				", max_size=" + std::to_string(max_size) + "): data is too small: " +
				std::to_string(array.size()) + " < " + std::to_string(wantSize)
		);
	}
	m_maxCount = max_size;
}

CompactUintArray::CompactUintArray(const CompactUintArray & other) :
RCWrapper< sserialize::CompactUintArrayPrivate >(other),
m_maxCount(other.m_maxCount)
{}

CompactUintArray::~CompactUintArray() {}

UByteArrayAdapter::OffsetType CompactUintArray::getSizeInBytes() const {
	uint64_t tmp = (uint64_t)m_maxCount*bpn();
	return tmp/8 + (tmp%8 ? 1 : 0);
}

CompactUintArray& CompactUintArray::operator=(const CompactUintArray & other) {
	RCWrapper< sserialize::CompactUintArrayPrivate >::operator=(other);
	m_maxCount = other.m_maxCount;
	return *this;
}

CompactUintArray::Bits CompactUintArray::bpn() const{
	return priv()->bpn();
}

CompactUintArray::SizeType CompactUintArray::findSorted(value_type key, SizeType len) const {
	if (len == 0) {
		return npos;
	}
	using signed_size = std::make_signed_t<SizeType>;
	
	signed_size left = 0;
	signed_size right = len-1;
	signed_size mid = (right-left)/2+left;
	value_type tk = priv()->at(mid);
	while( left < right ) {
		if (tk == key) {
			return mid;
		}
		if (tk < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
		tk = priv()->at(mid);
	}
	return (tk == key ? mid : npos);
}


CompactUintArray::value_type CompactUintArray::at(SizeType pos) const {
	if (UNLIKELY_BRANCH(pos >= m_maxCount)) {
		throw sserialize::OutOfBoundsException("CompactUintArray::at: maxCount=" + std::to_string(m_maxCount) + ", pos=" + std::to_string(pos));
	}
	return priv()->at(pos);
}

CompactUintArray::value_type CompactUintArray::at64(SizeType pos) const {
	return at(pos);
}

CompactUintArray::value_type CompactUintArray::set(const SizeType pos, const value_type value) {
	if (UNLIKELY_BRANCH(pos >= m_maxCount)) {
		throw sserialize::OutOfBoundsException("CompactUintArray::set: maxCount=" + std::to_string(m_maxCount) + ", pos=" + std::to_string(pos));
	}
	return priv()->set(pos, value);
}

CompactUintArray::value_type CompactUintArray::set64(const SizeType pos, const value_type value) {
	return set(pos, value);
}

bool CompactUintArray::reserve(SizeType newMaxCount) {
	if (maxCount() >= newMaxCount) {
		return true;
	}
	
	Bits mybpn = bpn();
	UByteArrayAdapter::OffsetType neededByteCount = minStorageBytes(mybpn, newMaxCount);
	UByteArrayAdapter::OffsetType haveByteCount = priv()->data().size();
	if (priv()->data().growStorage(neededByteCount-haveByteCount)) {
		m_maxCount = newMaxCount;
		return true;
	}
	return false;
}


UByteArrayAdapter & CompactUintArray::data() {
	return priv()->data();
}

const UByteArrayAdapter & CompactUintArray::data() const {
	return priv()->data();
}

std::ostream & CompactUintArray::dump(std::ostream& out, SizeType len) {
	if (maxCount() < len)
		len = maxCount();

	out << "(";
	for(SizeType i(0); i < len; i++) {
		out << at(i) << ", ";
	}
	out << ")" << std::endl;
	return out;
}

void CompactUintArray::dump() {
	dump(std::cout, maxCount());
}

CompactUintArray::const_iterator CompactUintArray::begin() const {
	return const_iterator(0, this);
}
	
CompactUintArray::const_iterator CompactUintArray::cbegin() const {
	return const_iterator(0, this);
}


bool CompactUintArray::operator!=(const CompactUintArray & other) const {
	return data() != other.data();
}

bool CompactUintArray::operator==(const CompactUintArray & other) const {
	return priv() == other.priv() || data() == other.data();
}

CompactUintArray::Bits CompactUintArray::minStorageBits(value_type number) {
	return std::max<Bits>(1, msb(number)+1);
}
CompactUintArray::Bits CompactUintArray::minStorageBitsFullBytes(value_type number) {
	Bits res = 0; 
	do {
		number >>= 8;
		res += 8;
	} while (number);
	return res;
}

UByteArrayAdapter::OffsetType CompactUintArray::minStorageBytes(Bits bpn, SizeType count) {
	SizeType bits = count*bpn;
	return bits/8 + UByteArrayAdapter::OffsetType((bits%8)>0);
}

BoundedCompactUintArray::BoundedCompactUintArray(const sserialize::UByteArrayAdapter & d) :
CompactUintArray(0)
{
	int len = -1;
	uint64_t sizeBits = d.getVlPackedUint64(0, &len);
	
	if (UNLIKELY_BRANCH(len < 0)) {
		throw sserialize::IOException("BoundedCompactUintArray::BoundedCompactUintArray");
	}
	
	m_size = sserialize::narrow_check<SizeType>(sizeBits >> 6);
	setMaxCount(m_size);
	
	Bits bits = (sizeBits & 0x3F) + 1;

	UByteArrayAdapter::OffsetType dSize = minStorageBytes(bits, m_size);
	if (m_size) {
		CompactUintArray::setPrivate(UByteArrayAdapter(d, (uint32_t)len, dSize), bits);
	}
	else {
		CompactUintArray::MyBaseClass::setPrivate(new CompactUintArrayPrivateEmpty());
	}
	
}

BoundedCompactUintArray::BoundedCompactUintArray(sserialize::UByteArrayAdapter & d, sserialize::UByteArrayAdapter::ConsumeTag) :
BoundedCompactUintArray(d)
{
	d += getSizeInBytes();
}

BoundedCompactUintArray::BoundedCompactUintArray(sserialize::UByteArrayAdapter const & d, sserialize::UByteArrayAdapter::NoConsumeTag) :
BoundedCompactUintArray(d)
{}

BoundedCompactUintArray::BoundedCompactUintArray(const BoundedCompactUintArray & other) :
CompactUintArray(other),
m_size(other.m_size)
{}

BoundedCompactUintArray::~BoundedCompactUintArray() {}

BoundedCompactUintArray & BoundedCompactUintArray::operator=(const BoundedCompactUintArray & other) {
	CompactUintArray::operator=(other);
	m_size = other.m_size;
	return *this;
}

UByteArrayAdapter::OffsetType  BoundedCompactUintArray::getSizeInBytes() const {
	Bits bits = bpn();
	uint64_t sb = (static_cast<uint64_t>(m_size) << 6) | (bits-1);
	return psize_vu64(sb) + minStorageBytes(bits, m_size);
}

std::ostream & operator<<(std::ostream & out, const BoundedCompactUintArray & src) {
	out << "BoundedCompactUintArray[size=" << src.size() << ", bpn=" << src.bpn() << "]{";
	if (src.size()) {
		out << src.at(0);
		for(SizeType i(1), s(src.size()); i < s; ++i) {
			out << "," << src.at(i);
		}
	}
	out << "}";
	return out;
}

}//end namespace
