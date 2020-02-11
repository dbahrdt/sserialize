#pragma once

#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/containers/RandomCache.h>
#include <sserialize/Static/ItemIndexStore.h>

#include <sserialize/spatial/dgg/SpatialGrid.h>
#include <sserialize/spatial/dgg/HCQRIndex.h>
#include <sserialize/spatial/dgg/HCQRSpatialGrid.h>


namespace sserialize::spatial::dgg {
namespace detail::HCQRIndexFromCellIndex::interface {

class CellIndex: public sserialize::RefCountObject {
public:
    using Self = CellIndex;
    using CellQueryResult = sserialize::CellQueryResult;
public:
    CellIndex() {}
    virtual ~CellIndex() {}
public:
    virtual sserialize::StringCompleter::SupportedQuerries getSupportedQueries() const = 0;
	virtual CellQueryResult complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const = 0;
	virtual CellQueryResult items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const = 0;
	virtual CellQueryResult regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const = 0;
public:
	virtual CellQueryResult cell(uint32_t cellId) const = 0;
	virtual CellQueryResult region(uint32_t regionId) const = 0;
};

class CellInfo: public sserialize::RefCountObject {
public:
    using SpatialGrid = sserialize::spatial::dgg::interface::SpatialGrid;
    using PixelId = SpatialGrid::PixelId;
    using CompressedPixelId = SpatialGrid::CompressedPixelId;
    using ItemIndex = sserialize::ItemIndex;
public:
    CellInfo() {}
    virtual ~CellInfo() {}
public:
    virtual SpatialGrid::Level level() const = 0;
public:
    virtual bool hasPixel(PixelId pid) const = 0;
	///should throw iff !hasPixel(pid)
    virtual ItemIndex items(PixelId pid) const = 0;
	virtual PixelId pixelId(CompressedPixelId const & cpid) const = 0;
public:
	virtual std::vector<CompressedPixelId> cells() const = 0;
};

}//end namespace detail::HCQRIndexFromCellIndex::interface

namespace detail::HCQRIndexFromCellIndex::impl {

class SpatialGridInfoFromCellIndex: public sserialize::spatial::dgg::interface::SpatialGridInfo {
public:
    using SizeType = uint32_t;
    using ItemIndex = sserialize::ItemIndex;

    using CellInfo = sserialize::spatial::dgg::detail::HCQRIndexFromCellIndex::interface::CellInfo;
    using SpatialGrid = sserialize::spatial::dgg::interface::SpatialGrid;

    using CellInfoPtr = sserialize::RCPtrWrapper<CellInfo>;
    using SpatialGridPtr = sserialize::RCPtrWrapper<SpatialGrid>;
public:
    SpatialGridInfoFromCellIndex(SpatialGridPtr const & sg, CellInfoPtr const & ci);
	~SpatialGridInfoFromCellIndex() override {}
    SizeType itemCount(PixelId pid) const override;
    ItemIndex items(PixelId pid) const override;
	PixelId pixelId(CompressedPixelId const & cpid) const override;
private:
    using PixelItemsCache = sserialize::RandomCache<PixelId, sserialize::ItemIndex>;
	struct Node {
		using size_type = uint32_t;
		static constexpr uint32_t npos = std::numeric_limits<size_type>::max();
		PixelId pid;
		size_type begin;
		size_type end;
		Node(PixelId pid, size_type begin, size_type end) : pid(pid), begin(begin), end(end) {}
		Node(PixelId pid, size_type begin) : Node(pid, begin, begin) {}
		Node(PixelId pid) : Node(pid, npos) {}
		inline size_type size() const { return end-begin; }
	};
	using Tree = std::vector<Node>;
protected:
	sserialize::ItemIndex items(Node const & node) const;
	Tree const & tree() const;
	Tree::const_iterator node(PixelId pid) const;
	SpatialGridPtr const & sg() const { return m_sg; }
	CellInfoPtr const & ci() const { return m_ci; }
private:
    SpatialGridPtr m_sg;
    CellInfoPtr m_ci;
	Tree m_t;
	mutable std::mutex m_cacheLock;
    mutable PixelItemsCache m_cache;
};

class SpatialGridInfoFromCellIndexWithIndex: public SpatialGridInfoFromCellIndex {
public:
	SpatialGridInfoFromCellIndexWithIndex(SpatialGridPtr const & sg, CellInfoPtr const & ci);
	~SpatialGridInfoFromCellIndexWithIndex() override {}
private:
	ItemIndex items(PixelId pid) const override;
private:
	using TreeItemsPtr = std::vector<sserialize::Static::ItemIndexStore::IdType>;
private:
	sserialize::Static::ItemIndexStore m_idxStore;
	TreeItemsPtr m_ti;
};

}//end namespace detail::HCQRIndexFromCellIndex::impl

class HCQRIndexFromCellIndex: public sserialize::spatial::dgg::interface::HCQRIndex {
public:
    using ItemIndexStore = sserialize::Static::ItemIndexStore;

    using CellIndex = sserialize::spatial::dgg::detail::HCQRIndexFromCellIndex::interface::CellIndex;
    using CellIndexPtr = sserialize::RCPtrWrapper<CellIndex>;

    using SpatialGridPtr = sserialize::RCPtrWrapper<SpatialGrid>;
    using SpatialGridInfoPtr = sserialize::RCPtrWrapper<SpatialGridInfo>;

    using MyHCQR = sserialize::spatial::dgg::impl::HCQRSpatialGrid;

    using PixelId = SpatialGrid::PixelId;
public:
    HCQRIndexFromCellIndex(
        SpatialGridPtr const & sg,
        SpatialGridInfoPtr const & sgi,
        CellIndexPtr const & ci
    );
    virtual ~HCQRIndexFromCellIndex();
public:
    sserialize::StringCompleter::SupportedQuerries getSupportedQueries() const override;
	HCQRPtr complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
	HCQRPtr items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
	HCQRPtr regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
public:
	HCQRPtr cell(uint32_t cellId) const override;
	HCQRPtr region(uint32_t regionId) const override;
public:
	SpatialGridInfo const & sgi() const override;
	SpatialGrid const & sg() const override;
public:
    CellIndex const & ci() const;
    ItemIndexStore const & idxStore() const;
private:
    SpatialGridPtr m_sg;
    SpatialGridInfoPtr m_sgi;
    CellIndexPtr m_ci;
};

}//end namespace sserialize::spatial::dgg

