#ifndef SSERIALIZE_DYNAMIC_BIT_SET_H
#define SSERIALIZE_DYNAMIC_BIT_SET_H
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {

class ItemIndex;

class DynamicBitSet {
	UByteArrayAdapter m_data;
public:
	///creates a DynamicBitSet with a in-memory cache as backend
	DynamicBitSet();
	///Create DynamicBitSet at the beginning of data
	DynamicBitSet(const sserialize::UByteArrayAdapter & data);
	virtual ~DynamicBitSet();
	///@param shift: number of bytes to align to expressed as aa power of two. i.e. 0 => 1 byte, 1 => 2 bytes, 2 => 4 bytes, 3 => 8 bytes
	bool align(uint8_t shift);
	
	DynamicBitSet operator&(const DynamicBitSet & other) const;
	DynamicBitSet operator|(const DynamicBitSet & other) const;
	DynamicBitSet operator-(const DynamicBitSet & other) const;
	DynamicBitSet operator^(const DynamicBitSet & other) const;
	DynamicBitSet operator~() const;
	
	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data;}

	bool isSet(uint32_t pos) const;
	void set(uint32_t pos);
	void unset(uint32_t pos);
	ItemIndex toIndex(int type) const;
	
	uint32_t size() const;
};

}//end namespace


#include <sserialize/containers/ItemIndex.h>
#endif