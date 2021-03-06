#ifndef SSERIALIZE_DYNAMIC_VECTOR_H
#define SSERIALIZE_DYNAMIC_VECTOR_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/Static/Array.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/storage/Size.h>

namespace sserialize {
namespace Static {

template<typename TPushValue, typename TGetValue = TPushValue>
class DynamicVector {
public:
	using SizeType = sserialize::Size;
public:
	SizeType m_size;
	sserialize::MMVector<OffsetType> m_offsets;
	UByteArrayAdapter m_data;
private:
	DynamicVector(const DynamicVector & other);
	DynamicVector & operator=(const DynamicVector & other);
public:
	DynamicVector(SizeType approxItemCount, OffsetType initalDataSize, sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED);
	virtual ~DynamicVector();
	void swap(DynamicVector & other);
	SizeType size() const;
	OffsetType reservedSize() const;
	void reserve(SizeType /*size*/) { std::cerr << "Reserving is not supported by sserialize::Static::DynamicVector" << std::endl; }
	template<typename TStreamingSerializer = UByteArrayAdapter::StreamingSerializer<TPushValue> >
	void push_back(const TPushValue & value, const TStreamingSerializer & serializer = TStreamingSerializer());
	void pop_back();
	UByteArrayAdapter & beginRawPush();
	void endRawPush();
	template<typename TDeserializer = UByteArrayAdapter::Deserializer<TGetValue> >
	TGetValue at(SizeType pos, const TDeserializer & derefer = TDeserializer()) const;
	UByteArrayAdapter dataAt(SizeType pos) const;
	
	UByteArrayAdapter & toArray(UByteArrayAdapter & dest) const;
};

template<typename TPushValue, typename TGetValue>
void swap(sserialize::Static::DynamicVector<TPushValue, TGetValue> & a, sserialize::Static::DynamicVector<TPushValue, TGetValue> & b) {
	return a.swap(b);
}

template<typename TPushValue, typename TGetValue>
DynamicVector<TPushValue, TGetValue>::DynamicVector(SizeType approxItemCount, OffsetType initalDataSize, sserialize::MmappedMemoryType mmt ) :
m_size(0),
m_offsets(mmt),
m_data(UByteArrayAdapter::createCache(initalDataSize, mmt))
{
	m_offsets.reserve(approxItemCount);
}


template<typename TPushValue, typename TGetValue>
DynamicVector<TPushValue, TGetValue>::~DynamicVector() {}

template<typename TPushValue, typename TGetValue>
void DynamicVector<TPushValue, TGetValue>::swap(DynamicVector & other) {
	using std::swap;
	swap(m_size, other.m_size);
	swap(m_offsets, other.m_offsets);
	swap(m_data, other.m_data);
}

template<typename TPushValue, typename TGetValue>
typename DynamicVector<TPushValue, TGetValue>::SizeType
DynamicVector<TPushValue, TGetValue>::size() const {
	return m_size;
}

template<typename TPushValue, typename TGetValue>
OffsetType DynamicVector<TPushValue, TGetValue>::reservedSize() const {
	return m_data.size();
}

template<typename TPushValue, typename TGetValue>
template<typename TStreamingSerializer>
void DynamicVector<TPushValue, TGetValue>::push_back(const TPushValue & value, const TStreamingSerializer & serializer) {
	serializer.operator()(value, beginRawPush());
	endRawPush();
}

template<typename TPushValue, typename TGetValue>
void DynamicVector<TPushValue, TGetValue>::pop_back() {
	if (m_size) {
		OffsetType tmp = m_offsets.at(m_size-1);
		m_offsets.pop_back();
		--m_size;
		if (!m_size) {
			m_data.setPutPtr(0);
		}
		else {
			m_data.setPutPtr(tmp);
		}
	}
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter & DynamicVector<TPushValue, TGetValue>::beginRawPush() {
	m_offsets.push_back(m_data.tellPutPtr());
	return m_data;
}

template<typename TPushValue, typename TGetValue>
void DynamicVector<TPushValue, TGetValue>::endRawPush() {
	++m_size;
}

template<typename TPushValue, typename TGetValue>
template<typename TDeserializer>
TGetValue DynamicVector<TPushValue, TGetValue>::at(SizeType pos, const TDeserializer & deserializer) const {
	if (pos >= size())
		return TGetValue();
	return deserializer(dataAt(pos));
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter DynamicVector<TPushValue, TGetValue>::dataAt(SizeType pos) const {
	if (pos >= size())
		return UByteArrayAdapter();
	OffsetType begin = m_offsets.at(pos);
	OffsetType len;
	if (pos == m_size-SizeType(1)) {
		len = m_data.tellPutPtr()-begin;
	}
	else {
		len = m_offsets.at(pos+1)-begin;
	}
	UByteArrayAdapter ret(m_data, begin, len);
	ret.resetPtrs();
	return ret;
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter & DynamicVector<TPushValue, TGetValue>::toArray(UByteArrayAdapter & dest) const {
	dest.putUint8(4); //Static::Array version
	dest.putOffset(m_data.size()); //datasize
	dest.putData(m_data);
	if constexpr (sserialize::SerializationInfo<TGetValue>::is_fixed_length) {
		if constexpr (!std::is_integral<TGetValue>::value) {
			dest.putVlPackedUint32(sserialize::SerializationInfo<TGetValue>::length);
		}
	}
	else {
		#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
		OffsetType oiBegin = dest.tellPutPtr();
		#endif
		if (!sserialize::Static::SortedOffsetIndexPrivate::create(m_offsets, dest)) {
			throw sserialize::CreationException("Array::flush: Creating the offset");
		}
		#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
		sserialize::UByteArrayAdapter tmp = dest;
		tmp.setPutPtr(oiBegin);
		tmp.shrinkToPutPtr();
		sserialize::Static::SortedOffsetIndex oIndex(tmp);
		if (oIndex != m_offsets) {
			throw sserialize::CreationException("Array::flush Offset index is unequal");
		}
		if (oIndex.getSizeInBytes() != (dest.tellPutPtr()-oiBegin)) {
			throw sserialize::CreationException("Array::flush Offset index reports wrong sizeInBytes()");
		}
		#endif
	}
	return dest;
}

}}//end namespace

template<typename TPushValue, typename TGetValue>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const sserialize::Static::DynamicVector<TPushValue, TGetValue> & src) {
	return src.toArray(dest);
}

#endif
