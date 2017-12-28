#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_PFOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_PFOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/assert.h>
#include <limits>

namespace sserialize {

class ItemIndexPrivatePFoR;

namespace detail {
namespace ItemIndexImpl {

class PForCreator;

/** A single frame of reference block
  * Data format is as follows:
  * --------------------------------
  * DATA            |OUTLIERS
  * --------------------------------
  * CompactUintArray|vu32*
  * 
  * Since the delta between two successive entries is at least 1,
  * a zero encodes an exception located in the outliers section
  * 
  */

class PFoRBlock final {
public:
	PFoRBlock();
	PFoRBlock(const PFoRBlock&) = default;
	PFoRBlock(PFoRBlock&&) = default;
	explicit PFoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn);
	~PFoRBlock();
	PFoRBlock & operator=(const PFoRBlock &) = default;
	PFoRBlock & operator=(PFoRBlock &&) = default;
	uint32_t size() const;
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
	uint32_t front() const;
	uint32_t back() const;
	uint32_t at(uint32_t pos) const;
public:
	template<typename T_OUTPUT_ITERATOR>
	SizeType decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn, T_OUTPUT_ITERATOR out);
private:
	std::vector<uint32_t> m_values;
	sserialize::UByteArrayAdapter::SizeType m_dataSize;
};

class PFoRIterator: public detail::AbstractArrayIterator<uint32_t> {
public:
	using MyBaseClass = detail::AbstractArrayIterator<uint32_t>;
public:
	PFoRIterator(const PFoRIterator &) = default;
	PFoRIterator(PFoRIterator &&) = default;
	virtual ~PFoRIterator() override;
public:
	virtual value_type get() const override;
	virtual void next() override;
	virtual bool notEq(const MyBaseClass * other) const override;
	virtual bool eq(const MyBaseClass * other) const override;
	virtual MyBaseClass * copy() const override;
private:
	friend class sserialize::ItemIndexPrivatePFoR;
private:
	///begin iterator
	explicit PFoRIterator(uint32_t idxSize, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data);
	///end iterator
	explicit PFoRIterator(uint32_t idxSize);
private:
	sserialize::UByteArrayAdapter m_data;
	sserialize::CompactUintArray m_bits;
	uint32_t m_indexPos;
	uint32_t m_indexSize;
	uint32_t m_blockPos;
	PFoRBlock m_block;
};

class PFoRCreator final {
public:
	PFoRCreator(const PFoRCreator& other) = delete;
	PFoRCreator & operator=(const PFoRCreator & other) = delete;
public:
	PFoRCreator();
	PFoRCreator(UByteArrayAdapter & data, uint32_t blockSizeOffset);
	PFoRCreator(UByteArrayAdapter & data, uint32_t finalSize, uint32_t blockSizeOffset);
	PFoRCreator(PFoRCreator && other);
	~PFoRCreator();
	uint32_t size() const;
	void push_back(uint32_t id);
	void flush();
	
	UByteArrayAdapter flushedData() const;
	///flush needs to be called before
	ItemIndex getIndex();
	///flush needs to be called before
	sserialize::ItemIndexPrivate * getPrivateIndex();
public:
	
	///begin->end are delta values
	template<typename T_ITERATOR>
	static uint32_t encodeBlock(sserialize::UByteArrayAdapter& dest, T_ITERATOR begin, T_ITERATOR end);

	///begin->end are delta values
	template<typename T_ITERATOR>
	static sserialize::SizeType storageSize(T_ITERATOR begin, T_ITERATOR end, uint32_t bits);
	
	///begin->end are delta values
	template<typename T_ITERATOR>
	static void optBits(T_ITERATOR begin, T_ITERATOR end, uint32_t & optBits, uint32_t & optStorageSize);
	
	///begin->end are absolute values
	template<typename T_ITERATOR>
	static uint32_t optBlockSize(T_ITERATOR begin, T_ITERATOR end);
private:
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;
	void flushBlock();
private:
	bool m_fixedSize;
	uint32_t m_size;
	uint32_t m_blockSizeOffset;
	//holds delta values!
	std::vector<uint32_t> m_values;
	uint32_t m_prev;
	std::vector<uint8_t> m_blockBits;
	UByteArrayAdapter m_data;
	UByteArrayAdapter * m_dest;
	sserialize::UByteArrayAdapter::OffsetType m_putPtr;
	bool m_delete;
};


}} //end namespace detail::ItemIndexImpl

/** Default format is:
  * 
  * struct {
  *     v_unsigned<32> size; //number of entries
  *     v_unsigned<32> dataSize; //size of the blockData section
  *     List<PFoRBlock> blocks; //pfor blocks
  *     //first entry is the size of a block given as offset into ItemIndexPrivatePFoR::BlockSizes
  *     //following entries encode the bit size of a block
  *     CompactUintArray<5> blockDesc;
  * }
  * 
  * where
  * SIZE is the number of entries
  * BLOCK_DATA contains the index encoded in PForBlocks
  * BLOCK_BITSIZES encodes the bpn of PFoRBlock as BLOCK_BITSIZE = bpn-1
  * BLOCK_SIZE is an offset into the block sizes array
  * 
  **/

class ItemIndexPrivatePFoR: public ItemIndexPrivate {
public:
	//maximum number of entries a single block may hold
	static constexpr uint32_t DefaultBlockSizeOffset = 9;
	static const std::array<uint32_t, 25> BlockSizes = {
		1, 2, 4, 8, 16, 32, 64, 128, 256, 512, //10 entries
		1024, 2048, 4096, 8192, 16384, 32768, 65536, //7 entries
		24, 48, 96, 192, 384, 768, 1536, 3072 // 8 entriess
	};
public:
	ItemIndexPrivatePFoR(const UByteArrayAdapter & d);
	virtual ~ItemIndexPrivatePFoR();
	
	virtual ItemIndex::Types type() const override;
	
	virtual UByteArrayAdapter data() const override;
public:
	///load all data into memory (only usefull if the underlying storage is not contigous)
	virtual void loadIntoMemory() override;

	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;
	
	virtual const_iterator cbegin() const override;
	virtual const_iterator cend() const override;

	virtual uint32_t size() const override;
	
	virtual uint8_t bpn() const override;
	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;

	virtual bool is_random_access() const override;

	virtual void putInto(DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t* dest) const override;

	///Default uniteK uses unite
	virtual ItemIndexPrivate * uniteK(const sserialize::ItemIndexPrivate * other, uint32_t numItems) const override;
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;
public:
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
	///create new index beginning at dest.tellPutPtr()
	template<typename T_ITERATOR>
	static bool create(T_ITERATOR begin, const T_ITERATOR&  end, sserialize::UByteArrayAdapter& dest);
	///create new index beginning at dest.tellPutPtr()
	template<typename TSortedContainer>
	static bool create(const TSortedContainer & src, UByteArrayAdapter & dest);
private:
	uint32_t firstId() const;
private:
	UByteArrayAdapter m_d;
	uint32_t m_size;
	uint32_t m_firstId;
	uint32_t m_blockSize;
	mutable AbstractArrayIterator<uint32_t> m_it;
	mutable std::vector<uint32_t> m_cache;
};

}//end namespace

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {


template<typename T_OUTPUT_ITERATOR>
sserialize::SizeType PFoRBlock::decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn, T_OUTPUT_ITERATOR out) {
	sserialize::SizeType dataSize = 0;
	CompactUintArray arr(d, bpn, size);
	dataSize += arr.getSizeInBytes();
	d.incGetPtr(s);
	for(uint32_t i(0), s(arr.getSizeInBytes(); i < s; ++i) {
		uint32_t v = arr.at(i);
		if (v == 0) {
			int len = -1;
			v = d.getVlPackedUint32(&len);
			SSERIALIZE_CHEAP_ASSERT_SMALLER(0, len);
			dataSize += len;
		}
		prev += v;
		*out = prev;
		++out;
	}
	return dataSize;
}

template<typename T_ITERATOR>
uint32_t PFoRCreator::encodeBlock(sserialize::UByteArrayAdapter & dest, T_ITERATOR begin, T_ITERATOR end) {
	using std::distance;
	std::size_t ds = distance(begin, end);
	uint32_t optBits, optStorageSize;
	PFoRCreator::optBits(begin, end, optBits, optStorageSize);
	std::vector<uint32_t> dv;
	std::vector<uint32_t> outliers;
	UByteArrayAdapter tmp(UByteArrayAdapter::createCache(CompactUintArray::minStorageBytes(optBits, ds)), MM_PROGRAM_MEMORY);
	for(T_ITERATOR it(begin); it != end; ++it) {
		if (CompactUintArray::minStorageBits(*it) > optBits) {
			dv.push_back(0);
			outliers.push_back(*it);
		}
		else {
			SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(0), *it);
			dv.push_back(*it);
		}
	}
	uint32_t resultBits = CompactUintArray::create(dv, dest);
	SSERIALIZE_CHEAP_ASSERT_EQUAL(optBits, resultBits);
	for(uint32_t x : outliers) {
		dest.putVlPackedUint32(x);
	}
	
	return resultBits;
}

template<typename T_ITERATOR>
sserialize::SizeType PFoRCreator::storageSize(T_ITERATOR begin, T_ITERATOR end, uint32_t bits) {
	using std::distance;
	std::size_t dist = distance(begin, end);
	sserialize::SizeType s = CompactUintArray::minStorageBytes(bits, dist);
	for(; begin != end; ++begin) {
		if ( CompactUintArray::minStorageBits(*begin) > bits) {
			s += sserialize::psize_vu32(*begin);
		}
	}
	return s;
}

template<typename T_ITERATOR>
void PFoRCreator::optBits(T_ITERATOR begin, T_ITERATOR end, uint32_t & optBits, uint32_t & optStorageSize) {
	using std::distance;
	std::size_t ds = distance(begin, end);
	
	if (ds < 1) {
		return 0;
	}
	
	uint32_t mindv = *begin;
	uint32_t maxdv = *begin;
	for(auto it(begin+1); begin != end; ++begin) {
		mindv = std::max<uint32_t>(mindv, *it);
		maxdv = std::max<uint32_t>(maxdv, *it);
	}
	
	uint32_t minbits = CompactUintArray::minStorageBits(mindv);
	uint32_t maxbits = CompactUintArray::minStorageBits(maxdv);
	optBits = maxbits;
	optStorageSize = std::numeric_limits<sserialize::SizeType>::max();
	for(uint32_t bits(minbits); bits <= maxbits; ++bits) {
		auto s = storageSize(begin, end, bits);
		if (s < optStorageSize) {
			optStorageSize = s;
			optBits = bits;
		}
	}
}

template<typename T_ITERATOR>
uint32_t PFoRCreator::optBlockSize(T_ITERATOR begin, T_ITERATOR end) {
	std::vector<uint32_t> tmp;
	{
		using std::distance;
		tmp.reserve(distance(begin, end));
		uint32_t prev = 0;
		for(auto it(begin); it != end; ++it) {
			tmp.push_back(*it - prev);
			prev = *it;
		}
	}
	uint32_t optBlockSizeOffset = ItemIndexPrivatePFoR::BlockSizes.size()-1;
	sserialize::SizeType optDataSize = std::numeric_limits<sserialize::SizeType>::max();
	for(uint32_t i(0), s(32); i < s; ++i) {
		uint32_t blockSize = ItemIndexPrivatePFoR::BlockSizes[i];
		uint32_t numFullBlocks = tmp.size()/blockSize;
		uint32_t numPartialBlocks = tmp.size()%blockSize > 0; // int(false)==0, int(true)==1
		sserialize::SizeType ds = CompactUintArray::minStorageBytes(5, 1+numFullBlocks+numPartialBlocks);
		uint32_t prev = 0;
		for(auto it(tmp.begin()), end(tmp.end()); it < end; it += blockSize) {
			auto blockEnd = it + std::min<std::ptrdiff_t>(blockSize, end-it);
			uint32_t myOptBits;
			sserialize::SizeType myBlockStorageSize;
			PFoRCreator::optBits(prev, it, blockEnd, myOptBits, myBlockStorageSize);
			ds += myBlockStorageSize;
			if (blockEnd < end) {
				prev = *blockEnd;
			}
		}
		if (ds < optDataSize) {
			optBlockSizeOffset = i;
			optDataSize = ds;
		}
	}
	return optBlockSizeOffset;
}

}} //end namespace detail::ItemIndexImpl


template<typename T_ITERATOR>
bool ItemIndexPrivatePFoR::create(T_ITERATOR begin, const T_ITERATOR & end, UByteArrayAdapter & dest) {
	SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(begin, end));
	using std::distance;
	detail::ItemIndexImpl::PForCreator creator(dest, distance(begin, end));
	for(; begin != end; ++begin) {
		creator.push(*begin);
	}
	creator.flush();
	return true;
}

template<typename TSortedContainer>
bool
ItemIndexPrivatePFoR::create(const TSortedContainer & src, UByteArrayAdapter & dest) {
	return create(src.begin(), src.end(), dest);
}

}

#endif