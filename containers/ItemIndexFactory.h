#ifndef ITEM_INDEX_FACTORY_H
#define ITEM_INDEX_FACTORY_H
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/types.h>


namespace sserialize {

/** This class is a storage for multiple ItemIndex. It can create a file suitable for the Static::IndexStore */

class ItemIndexFactory {
private:
	uint32_t m_indexIdCounter;
	UByteArrayAdapter m_header;
	UByteArrayAdapter m_indexStore;
	std::unordered_map<uint64_t, std::list<OffsetType> > m_hash;
	std::unordered_map<uint64_t, uint32_t> m_offsetsToId;
	uint32_t m_hitCount;
	bool m_checkIndex;
	int8_t m_bitWidth;
	bool m_useRegLine;
	ItemIndex::Types m_type;
	
	uint64_t hashFunc(const std::vector< uint8_t >& v);
	int64_t getIndex(const std::vector< uint8_t >& v, uint64_t & hv);
	bool indexInStore(const std::vector< uint8_t >& v, uint64_t offset);
	
public:
	ItemIndexFactory(bool memoryBase = false);
	~ItemIndexFactory();
	uint32_t size() { return m_indexIdCounter;}
	UByteArrayAdapter at(OffsetType offset) const;
	///Sets the type. should not be called after having added indices
	void setType(ItemIndex::Types type) { m_type = type;}
	//create the index Store at the beginning of data
	void setIndexFile(UByteArrayAdapter data);
	
	void setCheckIndex(bool checkIndex) { m_checkIndex = checkIndex;}
	void setBitWith(int8_t bitWidth) { m_bitWidth = bitWidth; }
	void setRegline(bool useRegLine) { m_useRegLine = useRegLine; }
	
	uint32_t addIndex(const std::vector<uint8_t> & idx, OffsetType * indexOffset = 0);

	template<class TSortedContainer>
	uint32_t addIndex(const TSortedContainer & idx, bool * ok = 0, OffsetType * indexOffset = 0) {
		bool mok = false;
		std::vector<uint8_t> s;
		if (m_type == ItemIndex::T_REGLINE)
			mok = ItemIndexPrivateRegLine::create(idx, s, m_bitWidth, m_useRegLine);
		else if (m_type == ItemIndex::T_WAH) {
			mok = true;
			UByteArrayAdapter dest(&s);
			ItemIndexPrivateWAH::create(idx, dest);
		}
		else if (m_type == ItemIndex::T_DE) {
			mok = true;
			UByteArrayAdapter dest(&s);
			ItemIndexPrivateDE::create(idx, dest);
		}
		else if (m_type == ItemIndex::T_RLE_DE) {
			mok = true;
			UByteArrayAdapter dest(&s);
			ItemIndexPrivateRleDE::create(idx, dest);
		}
		if (ok)
			*ok = mok;
		if (mok)
			return addIndex(s, indexOffset);
		else
			return 0;
	}
	
	uint32_t addIndex(const std::unordered_set<uint32_t> & idx, bool * ok = 0, OffsetType * indexOffset = 0);

	
	inline UByteArrayAdapter & getIndexStore() { return m_indexStore;}
	inline uint32_t hitCount() { return m_hitCount; }
	
	UByteArrayAdapter getFlushedData();
	
	///Flushes the data, don't add indices afterwards
	///@return number of bytes from the beginning og the indexFile
	OffsetType flush();

};

}//end namespace

#endif
