#ifndef SSERIALIZE_DYNAMIC_BIT_SET_H
#define SSERIALIZE_DYNAMIC_BIT_SET_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/AbstractArray.h>
#include <sserialize/containers/ItemIndex.h>

namespace sserialize {

class DynamicBitSet;

namespace detail {
namespace DynamicBitSet {

class DynamicBitSetIdIterator: public sserialize::detail::AbstractArrayIterator<SizeType> {
public:
	DynamicBitSetIdIterator();
	virtual ~DynamicBitSetIdIterator();
	virtual SizeType get() const override;
	virtual void next() override;
	virtual bool notEq(const AbstractArrayIterator<SizeType> * other) const override;
	virtual bool eq(const AbstractArrayIterator<SizeType> * other) const override;
	virtual AbstractArrayIterator<SizeType> * copy() const override;
private:
	friend class sserialize::DynamicBitSet;
private:
	///@param p has to be valid
	DynamicBitSetIdIterator(const sserialize::DynamicBitSet * p, SizeType offset);
	void moveToNextSetBit();
private:
	const sserialize::DynamicBitSet * m_p;
	UByteArrayAdapter::OffsetType m_off;
	SizeType m_curId; 
	uint8_t m_d;
	uint8_t m_remainingBits;
};


}}

class DynamicBitSet final {
public:
	///An iterator that iterates over the set ids
	typedef AbstractArrayIterator<SizeType> const_iterator;
private:
	UByteArrayAdapter m_data;
public:
	///creates a DynamicBitSet with a in-memory cache as backend
	DynamicBitSet();
	DynamicBitSet(const DynamicBitSet & other) = delete;
	DynamicBitSet(DynamicBitSet && other) = default;
	///Create DynamicBitSet at the beginning of data
	DynamicBitSet(const sserialize::UByteArrayAdapter & data);
	~DynamicBitSet();
	DynamicBitSet & operator=(const DynamicBitSet & other) = delete;
	DynamicBitSet & operator=(DynamicBitSet && other) = default;
	void resize(UByteArrayAdapter::OffsetType size);
	///@param shift: number of bytes to align to expressed as a power of two. i.e. 0 => 1 byte, 1 => 2 bytes, 2 => 4 bytes, 3 => 8 bytes
	bool align(uint8_t shift);
	
	IdType smallestEntry() const;
	IdType largestEntry() const;
	//a good upper bound for the largest Entry
	IdType upperBound() const;
	
	bool operator==(const DynamicBitSet & other) const;
	bool operator!=(const DynamicBitSet & other) const;
	
	DynamicBitSet operator&(const DynamicBitSet & other) const;
	DynamicBitSet operator|(const DynamicBitSet & other) const;
	DynamicBitSet operator-(const DynamicBitSet & other) const;
	DynamicBitSet operator^(const DynamicBitSet & other) const;
	DynamicBitSet operator~() const;
	
	DynamicBitSet & operator&=(const DynamicBitSet & other);
	DynamicBitSet & operator|=(const DynamicBitSet & other);
	DynamicBitSet & operator-=(const DynamicBitSet & other);
	DynamicBitSet & operator^=(const DynamicBitSet & other);
	
	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data;}

	bool isSet(SizeType pos) const;
	void set(SizeType pos);
	template<typename T_IT>
	void set(T_IT begin, T_IT end) {
		for(; begin != end; ++begin) {
			set(*begin);
		}
	}
	void unset(SizeType pos);
	ItemIndex toIndex(int type, ItemIndex::CompressionLevel = ItemIndex::CL_DEFAULT) const;
	
	template<typename T_OUTPUT_ITERATOR>
	void putInto(T_OUTPUT_ITERATOR out) const {
		UByteArrayAdapter::OffsetType s = m_data.size();
		UByteArrayAdapter::OffsetType i = 0;
		SizeType id = 0;
		for(; i < s; ++i) {
			uint8_t d = m_data.getUint8(i);
			for(SizeType curId(id); d; ++curId, d >>= 1) {
				if (d & 0x1) {
					*out = curId;
					++out;
				}
			}
			id += 8;
		}
	}
	
	SizeType size() const;
	
	const_iterator cbegin() const;
	const_iterator cend() const;
};

}//end namespace


#include <sserialize/containers/ItemIndex.h>
#endif
