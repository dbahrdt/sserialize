#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/iterator/UDWIterator.h>
#include <sserialize/iterator/UDWIteratorPrivateHD.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/stats/ProgressInfo.h>
#include <minilzo/minilzo.h>
#include <mutex>
#include <sserialize/mt/ThreadPool.h>

namespace sserialize {
namespace Static {

template<>
ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::FixedLengthTag) :
m_d(d.getOffset(0))
{}

template<>
ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::VariableLengthTag) :
m_d(d.getVlPackedUint64(0))
{}

ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::SerializationType t) :
ItemIndexStoreId()
{
	switch(t) {
	case ST_FL:
		m_d = d.getOffset(0);
		break;
	case ST_VL:
		m_d = d.getVlPackedUint64(0);
		break;
	default:
		break;
	}
}

template<>
sserialize::UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::FixedLengthTag) const {
	return sserialize::UByteArrayAdapter::OffsetTypeSerializedLength();
}

template<>
sserialize::UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::VariableLengthTag) const {
	return sserialize::psize_vu64(m_d);
}

UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::SerializationType t) const {
	switch(t) {
	case ST_FL:
		return getSizeInBytes<ItemIndexStoreId::FixedLengthTag>();
	case ST_VL:
		return getSizeInBytes<ItemIndexStoreId::VariableLengthTag>();
	default:
		return 0;
	}
}

ItemIndexStore::ItemIndexStore() : m_priv(new detail::ItemIndexStore()) {}

ItemIndexStore::ItemIndexStore(const sserialize::UByteArrayAdapter & data) :
m_priv(new detail::ItemIndexStore(data))
{}

ItemIndexStore::ItemIndexStore(sserialize::UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag t) :
m_priv(new detail::ItemIndexStore(data, t))
{}

ItemIndexStore::ItemIndexStore(sserialize::UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag t) :
m_priv(new detail::ItemIndexStore(data, t))
{}

ItemIndexStore::ItemIndexStore(interfaces::ItemIndexStore * base) : m_priv(base) {}

namespace detail {

ItemIndexStore::LZODecompressor::LZODecompressor() {}

ItemIndexStore::LZODecompressor::LZODecompressor(const sserialize::UByteArrayAdapter & data) :
m_data(data)
{}

ItemIndexStore::LZODecompressor::~LZODecompressor() {}

OffsetType ItemIndexStore::LZODecompressor::getSizeInBytes() const {
	return m_data.getSizeInBytes();
}

UByteArrayAdapter ItemIndexStore::LZODecompressor::decompress(uint32_t id, const UByteArrayAdapter & src) const {
	if (m_data.maxCount() >= id) {
		uint32_t chunkLength = m_data.at(id);
		//Get a memory view, but we don't write to it, so it's ok
		const UByteArrayAdapter::MemoryView srcD = src.getMemView(0, src.size());
		uint8_t * dest = new uint8_t[chunkLength];

		lzo_uint destLen = chunkLength;
		int ok = ::lzo1x_decompress_safe(srcD.get(), src.size(), dest, &destLen, 0);
		if (ok != LZO_E_OK) {
			return UByteArrayAdapter();
		}
		else {
			UByteArrayAdapter ret(dest, 0, destLen);
			ret.setDeleteOnClose(true);
			return ret;
		}
	}
	return UByteArrayAdapter();
}

ItemIndexStore::ItemIndexStore() :
m_type(ItemIndex::T_EMPTY),
m_compression(sserialize::Static::ItemIndexStore::IC_NONE)
{}

ItemIndexStore::ItemIndexStore(UByteArrayAdapter data) :
m_version(data.getUint8(0))
{
	if (m_version == 5 || m_version == 6) {
		m_type = data.getUint16(1);
		m_compression = IndexCompressionType(data.getUint8(3));
		data.resetGetPtr();
		data += 4;
	}
	else if (m_version == 4) {
		m_type = data.getUint8(1);
		m_compression = IndexCompressionType(data.getUint8(2));
		data.resetGetPtr();
		data += 3;
	}
	else {
		SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION, m_version, "Static::ItemIndexStore");
	}
	OffsetType dataLength = data.getOffset();
	data.shrinkToGetPtr();
	
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(dataLength, data.size());
	
	if ( sserialize::popCount(m_type) > 1) {
		m_type |= ItemIndex::T_MULTIPLE;
	}
	
	m_data = UByteArrayAdapter(data, 0, dataLength);
	data += dataLength;
	
	m_index = SortedOffsetIndex(data);
	data += m_index.getSizeInBytes();
	
	m_idxSizes = sserialize::Static::Array<uint32_t>(data);
	data += m_idxSizes.getSizeInBytes();
	
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		m_hd.reset(new HuffmanDecoder(data) );
		data += m_hd->getSizeInBytes();
	}
	
	if  (m_compression & sserialize::Static::ItemIndexStore::IC_LZO) {
		if (m_version < 6) {
			throw sserialize::VersionMissMatchException("LZO compressed ItemIndexStore needs at least version 6", 6, m_version);
		}
		m_lzod.reset(new LZODecompressor(data));
		data += m_lzod->getSizeInBytes();
	}
	
	if (m_type & sserialize::ItemIndex::T_MULTIPLE) {
		uint32_t bits = sserialize::msb(sserialize::msb(uint32_t(m_type - sserialize::ItemIndex::T_MULTIPLE))) + 1;
		m_idxTypeInfo = CompactUintArray(data, bits, size());
		data += m_idxTypeInfo.getSizeInBytes();
	}
}

ItemIndexStore::ItemIndexStore(sserialize::UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag) :
ItemIndexStore(data)
{
	data += getSizeInBytes();
}

ItemIndexStore::ItemIndexStore(sserialize::UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag) :
ItemIndexStore(data)
{}

ItemIndexStore::~ItemIndexStore() {}

uint32_t ItemIndexStore::size() const {
	return m_index.size();
}

int ItemIndexStore::indexTypes() const {
	return m_type;
}

ItemIndex::Types ItemIndexStore::indexType(uint32_t pos) const {
	if (m_type & sserialize::ItemIndex::T_MULTIPLE) {
		return ItemIndex::Types(1 << m_idxTypeInfo.at(pos));
	}
	return ItemIndex::Types(m_type);
}

OffsetType ItemIndexStore::getSizeInBytes() const {
	OffsetType r = (m_version >= 5 ? 9 : 8);
	r += m_data.size();
	r += m_index.getSizeInBytes();
	r += m_idxSizes.getSizeInBytes();
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		r += m_hd->getSizeInBytes();
	}
	if (m_compression & sserialize::Static::ItemIndexStore::IC_LZO) {
		r += m_lzod->getSizeInBytes();
	}
	if (m_type & sserialize::ItemIndex::T_MULTIPLE) {
		r += m_idxTypeInfo.getSizeInBytes();
	}
	return r;
}

UByteArrayAdapter::OffsetType ItemIndexStore::dataSize(uint32_t pos) const {
	if (pos >= size())
		return 0;
	UByteArrayAdapter::OffsetType indexStart = m_index.at(pos);
	UByteArrayAdapter::OffsetType indexLength;
	if (pos+1 == size()) {
		indexLength = m_data.size() - indexStart;
	}
	else {
		indexLength = m_index.at(pos+1) - indexStart;
	}
	return indexLength;
}

UByteArrayAdapter ItemIndexStore::rawDataAt(uint32_t pos) const {
	if (pos >= size())
		return UByteArrayAdapter();
	UByteArrayAdapter::OffsetType indexStart = m_index.at(pos);
	UByteArrayAdapter::OffsetType indexLength;
	if (pos+1 == size()) {
		indexLength = m_data.size() - indexStart;
	}
	else {
		indexLength = m_index.at(pos+1) - indexStart;
	}
	return UByteArrayAdapter::makeContigous(UByteArrayAdapter(m_data, indexStart, indexLength));
}

ItemIndex ItemIndexStore::at(uint32_t pos) const {
	if (pos >= size()) {
		return ItemIndex();
	}
	
	ItemIndex::Types type = indexType(pos);
	
	UByteArrayAdapter idxData = rawDataAt(pos);
	if (m_compression & sserialize::Static::ItemIndexStore::IC_LZO) {
		idxData = m_lzod->decompress(pos, idxData);
	}
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		if (type == ItemIndex::T_WAH) {
			return ItemIndex::createInstance<ItemIndexPrivateWAH>(UDWIterator(new UDWIteratorPrivateHD(MultiBitIterator(idxData), m_hd), true));
		}
		else if (type == ItemIndex::T_RLE_DE) {
			return ItemIndex::createInstance<ItemIndexPrivateRleDE>(UDWIterator(new UDWIteratorPrivateHD(MultiBitIterator(idxData), m_hd), true));
		}
	}
	else if (m_compression & sserialize::Static::ItemIndexStore::IC_VARUINT32) {
		if (type == ItemIndex::T_WAH) {
			return ItemIndex::createInstance<ItemIndexPrivateWAH>(UDWIterator( new UDWIteratorPrivateVarDirect(idxData), true));
		}
	}
	else { //no further compression
		return ItemIndex(idxData, type);
	}
	return ItemIndex();
}

UByteArrayAdapter ItemIndexStore::getHuffmanTreeData() const {
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		UByteArrayAdapter data = getData();
		UByteArrayAdapter::OffsetType totalSize = data.size() + m_index.size();
		data.growStorage(m_index.getSizeInBytes() + m_hd->getSizeInBytes());
		data.incGetPtr(totalSize);
		data.shrinkToGetPtr();
		return data;
	}
	return UByteArrayAdapter();
}

std::ostream& ItemIndexStore::printStats(std::ostream& out) const {
	return printStats(out, [](uint32_t) { return true; });
}

std::ostream& ItemIndexStore::printStats(std::ostream& out, std::function<bool(uint32_t)> filter) const {
	out << "Static::ItemIndexStore::Stats->BEGIN (info depends on selected indices)" << std::endl;
	out << "Storage size  of ItemIndexStore: " << getSizeInBytes() << std::endl;
	
	struct State {
		uint32_t size;
		std::atomic<uint32_t> i;
		std::mutex lock;
		ProgressInfo pinfo;
	} state;
	state.size = this->size();
	state.i = 0;

	struct Stats {
		uint32_t numIdx = 0;
		uint32_t maxSizeIdx = 0;
		sserialize::SizeType maxSpaceIdx = 0;
		
		sserialize::SizeType siStorageSize = 0;
		uint64_t siSummedIdxSize = 0;
		
		std::array<uint32_t, 32> sizeHisto;
		
		std::vector<uint32_t> idFreqs;
		
		//pfor index stats
		struct PFoR {
			std::array<uint32_t, ItemIndexPrivatePFoR::BlockSizes.size()> bsOcc;
			std::array<uint32_t, 32> bcHisto;
			PFoR() { 
				bsOcc.fill(0);
				bcHisto.fill(0);
			}
			void update(const sserialize::ItemIndex & idx, const std::vector<uint32_t> & /*data*/) {
				const sserialize::ItemIndexPrivatePFoR * pidx = static_cast<const sserialize::ItemIndexPrivatePFoR*>(idx.priv());
				bsOcc.at(pidx->blockSizeOffset()) += 1;
				bcHisto.at( sserialize::msb(pidx->blockCount()) ) += 1;
			}
			void update(const PFoR & other) {
				for(std::size_t i(0), s(bsOcc.size()); i < s; ++i) {
					bsOcc[i] += other.bsOcc[i];
				}
				for(std::size_t i(0), s(bcHisto.size()); i < s; ++i) {
					bcHisto[i] += other.bcHisto[i];
				}
			}
		} pfor;
		
		Stats() {
			sizeHisto.fill(0);
		}
		
		void update(const sserialize::ItemIndex & idx, const std::vector<uint32_t> & data) {
			auto idxStorageSize = idx.getSizeInBytes();
			numIdx += 1;
			sizeHisto.at( sserialize::msb(idx.size()) ) += 1;
			maxSizeIdx = std::max<uint32_t>(maxSizeIdx, idx.size());
			maxSpaceIdx = std::max(maxSpaceIdx, idxStorageSize);
			siStorageSize += idxStorageSize;
			siSummedIdxSize += data.size();
			
			for(uint32_t x : data)  {
				if (x >= idFreqs.size()) {
					idFreqs.resize(x+1, 0);
				}
				idFreqs[x] += 1;
			}
			if (idx.type() == ItemIndex::T_PFOR) {
				pfor.update(idx, data);
			}
		}
		
		void update(const Stats & other) {
			numIdx += other.numIdx;
			maxSizeIdx = std::max(maxSizeIdx, other.maxSizeIdx);
			maxSpaceIdx = std::max(maxSpaceIdx, other.maxSpaceIdx);
			siStorageSize += other.siStorageSize;
			siSummedIdxSize += other.siSummedIdxSize;
			for(std::size_t i(0), s(sizeHisto.size()); i < s; ++i) {
				sizeHisto[i] += other.sizeHisto[i];
			}
			idFreqs.resize(std::max(idFreqs.size(), other.idFreqs.size()), 0);
			for(std::size_t i(0), s(other.idFreqs.size()); i < s; ++i) {
				idFreqs[i] += other.idFreqs[i];
			}
			pfor.update(other.pfor);
		}
		
		void print(std::ostream & out) {
			out << "#selected indices: " << numIdx << std::endl;
			out << "max(idx.size): " << maxSizeIdx << std::endl;
			out << "max(idx.getSizeInBytes): " << maxSpaceIdx << std::endl;
			out << "Size histogram [i>v]: ";
			for(std::size_t i(0), s(sizeHisto.size()); i < s; ++i) {
				if (sizeHisto.at(i)) {
					if (i<15) {
						out << (uint32_t(1) << (i+1));
					}
					else {
						out << "2^" << i+1;
					}
					out << ": " << 100*double(sizeHisto.at(i))/numIdx << ", ";
				}
			}
			out << std::endl;
			out << "Summed idx.size()):" << siSummedIdxSize << std::endl;
			out << "Storage size: " << siStorageSize << std::endl;
			out << "Mean bits/id (headers included): " << double(siStorageSize*8)/siSummedIdxSize << std::endl;
			long double idEntropy = 0;
			long double idDiscreteEntropy = 0;
			for(auto it(idFreqs.begin()), end(idFreqs.end()); it != end; ++it) {
				if (*it) {
					long double wn = double(*it)/siSummedIdxSize;
					long double log2res = logTo2(wn);
					idEntropy += wn * log2res;
					idDiscreteEntropy += wn * ceil( - log2res );
				}
			}
			idEntropy = -idEntropy;
			out << "Id entropy: " << idEntropy << std::endl;
			out << "Id discrete entropy: " << idDiscreteEntropy << std::endl;
			out << "ItemIndexPFoR::BlockSize histogram: ";
			for(std::size_t i(0), s(pfor.bsOcc.size()); i < s; ++i) {
				if (pfor.bsOcc.at(i)) {
					out << ItemIndexPrivatePFoR::BlockSizes.at(i) << ": " << 100*double(pfor.bsOcc.at(i))/numIdx << ", ";
				}
			}
			out << std::endl;
			out << "ItemIndexPFoR block count histogram: ";
			for(std::size_t i(0), s(pfor.bcHisto.size()); i < s; ++i) {
				if (pfor.bcHisto.at(i)) {
					if (i<15) {
						out << (uint32_t(1) << (i+1));
					}
					else {
						out << "2^" << i+1;
					}
					out << ": " << 100*double(pfor.bcHisto.at(i))/numIdx << ", ";
				}
			}
			out << std::endl;
		}
	} stats;
	
	struct Worker {
		State * state;
		Stats * globalStats;
		const ItemIndexStore * store;
		std::function<bool(uint32_t)> filter;
	
		Stats stats;
		std::vector<uint32_t> buffer;
		Worker(State * state, Stats * stats, const ItemIndexStore * store, const std::function<bool(uint32_t)> & filter) :
		state(state),
		globalStats(stats),
		store(store),
		filter(filter)
		{}
		Worker(const Worker & other) :
		state(other.state),
		globalStats(other.globalStats),
		store(other.store),
		filter(other.filter)
		{}
		void operator()() {
			while(true) {
				uint32_t idxId = state->i.fetch_add(1, std::memory_order_relaxed);
				if (idxId >= state->size) {
					break;
				}
				if ( !filter(idxId) ) {
					continue;
				}
				if (idxId % 1000 == 0) {
					state->pinfo(idxId);
				}
				sserialize::ItemIndex idx = store->at(idxId);
				idx.putInto(buffer);
				stats.update(idx, buffer);
				buffer.clear();
			}
			std::lock_guard<std::mutex> lck(state->lock);
			globalStats->update(stats);
		}
	};
	
	state.pinfo.begin(size(), "ItemIndexStore::calcStats");
	uint32_t threadCount = std::max<uint32_t>(1, std::thread::hardware_concurrency()/2);
	sserialize::ThreadPool::execute(Worker(&state, &stats, this, filter), threadCount, sserialize::ThreadPool::CopyTaskTag());
	state.pinfo.end();
	
	stats.print(out);
	out << "Static::ItemIndexStore::Stats->END" << std::endl;
	return out;
}


}}}//end namespace
