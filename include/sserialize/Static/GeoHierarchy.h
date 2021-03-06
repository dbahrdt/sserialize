#ifndef SSERIALIZE_STATIC_GEO_HIERARCHY_H
#define SSERIALIZE_STATIC_GEO_HIERARCHY_H
#include <sserialize/Static/Array.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/TreedCQR.h>
#include <unordered_map>

#define SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION 11

namespace sserialize {
namespace Static {
namespace spatial {

/**
  * Version 1: initial draft
  * Version 2: use offset array for child/parent ptrs, use MVBitArray for node description
  * Version 3: use offset array for cell parents, use MVBitArray for cell description
  * Version 4: add boundary to regions
  * Version 5: add items pointer to regions
  * Version 6: add items count to regions
  * Version 7: add items count to cells
  * Version 8: add sparse parent ptrs to cells
  * Version 9: add storeIdToGhId map
  * Version 10: add cell boundaries
  * Version 11: add region neighbor pointers
  *-------------------------------------------------------------------------------------------------------------------
  *VERSION|StoreIdToGhId|RegionDesc|RegionPtrs |RegionBoundaries        |CellDesc  |CellPtrs   |CellBoundaries    
  *--------------------------------------------------------------------------------------------------------------------
  *  1Byte|Array<u32>   |MVBitArray|BCUintArray|(regions.size+1)*GeoRect|MVBitArray|BCUintArray|Static::Array<GeoRect>
  *
  * There has to be one Region more than used. The last one defines the end for the RegionPtrs
  *
  *
  *Region
  *------------------------------------------------------------------------------------
  *CellListPtr|type|id|ChildrenBegin|ParentsOffset|NeighborsOffset|ItemsPtr|ItemsCount
  *------------------------------------------------------------------------------------
  * 
  * ParentBegin = Region[i+1].childrenBegin+Region[i].ParentsOffset
  * And the number of parents = NeighborsOffset
  * And the number of neighbors = Region[i+1].childrenBegin - (Region[i].childrenBegin + Region[i].ParentsOffset + Region[i].NeighBorsOffset)
  *
  *
  *Cell
  *--------------------------------
  *ItemPtrs|ItemCount|ParentsBegin
  *--------------------------------
  *
  * RegionPointers order of a single Region: [children][parents][neighbors]
  *
  * The last region is the root of the dag. it has no representation in a db and is not directly accessible
  * If a Region has no parent and is not the root region, then it is a child of the root region
  * In essence, the root region is just the entrypoint to the graph
  *
  *
  * Cell parent ptrs:
  * They consist of the direct parents of the cell and the union of the ancestors of the direct parents.
  * The can be accessed seperately.
  * This only useful during SubSet creation:
  * the direct parents are used in conjunction with SuBSet without aproximate region item counts and a recursive flattening strategy
  * the union is used if the SubSet needs an aproximate item counts and a single-region only flattening strategy (all contained cells are stored in the region)
  *
  */

class GeoHierarchy;

namespace detail {

///SubSet represents a SubSet of a GeoHierarchy. It's based on a CellQueryResult and has 2 incarnation
///A sparse SubSet has cellpositions/apxitemcount only for direct parents
///The id of the root-region equals GeoHierarchy::npos

class SubSetBase {
public:
	class Node: public RefCountObject {
	public:
		typedef RCPtrWrapper<Node> NodePtr;
		typedef std::vector<NodePtr> ChildrenStorageContainer;
		typedef std::vector<uint32_t> CellPositionsContainer;
		typedef ChildrenStorageContainer::iterator iterator;
		typedef ChildrenStorageContainer::const_iterator const_iterator;
	private:
		ChildrenStorageContainer m_children;
		uint32_t m_ghId;
		uint32_t m_itemSize;
		CellPositionsContainer m_cellPositions;
	private:
		template<typename TP>
		void visitImp(TP & p) {
			p(*this);
			for(auto x : *this) {
				x->visit(p);
			}
		}
		template<typename TP>
		void visitImp(TP & p) const {
			p(*this);
			for(auto x : *this) {
				x->visit(p);
			}
		}
	public:
		Node(const Node &) = delete;
		const Node & operator=(const Node &) = delete;
	public:
		Node(uint32_t ghId, uint32_t itemSize) : m_ghId(ghId), m_itemSize(itemSize) {}
		virtual ~Node() {}
		inline uint32_t ghId() const { return m_ghId; }
		//number of chilren
		inline std::size_t size() const { return m_children.size(); }
		inline NodePtr & operator[](uint32_t pos) { return m_children[pos]; }
		inline NodePtr & at(uint32_t pos) { return m_children.at(pos);}
		inline const NodePtr & operator[](uint32_t pos) const { return m_children[pos]; }
		inline const NodePtr & at(uint32_t pos) const { return m_children.at(pos);}
		inline void push_back(Node * child) { m_children.push_back( RCPtrWrapper<Node>(child) );}
		inline uint32_t maxItemsSize() const { return m_itemSize; }
		inline uint32_t & maxItemsSize() { return m_itemSize; }
		///A sparse SubSet has cellpositions/apxitemcount only for direct parents
		inline const CellPositionsContainer & cellPositions() const { return m_cellPositions;}
		///A sparse SubSet has cellpositions/apxitemcount only for direct parents
		inline CellPositionsContainer & cellPositions() { return m_cellPositions;}
		inline iterator begin() { return m_children.begin();}
		inline const_iterator begin() const { return m_children.begin();}
		inline const_iterator cbegin() const { return m_children.cbegin();}
		inline iterator end() { return m_children.end(); }
		inline const_iterator end() const { return m_children.end(); }
		inline const_iterator cend() const { return m_children.cend(); }
		void dump(std::ostream & out);
		void dump();
		template<typename TP>
		inline void visit(TP p) { visitImp(p); }
		template<typename TP>
		inline void visit(TP p) const { visitImp(p); }
	};
	typedef Node::NodePtr NodePtr;
};

class SubSet;

class FlatSubSet {
public:
	struct Node {
		uint32_t m_id;
		uint32_t m_childrenBegin;
		uint32_t m_cellsBegin;
		uint32_t m_maxItems;
	};
	typedef std::vector<Node> NodesContainer;
	typedef std::vector<uint32_t> NodePtrContainer;
	typedef std::vector<uint32_t> CellsContainer;
	typedef NodePtrContainer::const_iterator NodeChilrenIterator;
	typedef CellsContainer::const_iterator CellsIterator;
private:
	struct DataHolder: RefCountObject {
		NodesContainer nodes;
		NodePtrContainer nodePtrs;
		CellsContainer cells;
		bool sparse;
		DataHolder(bool sparse) : sparse(sparse) {}
	};
	RCPtrWrapper<DataHolder> m_d;
public:
	FlatSubSet(bool sparse) : m_d(new DataHolder(sparse)) {}
	~FlatSubSet() {}
	inline NodesContainer & nodes() { return m_d->nodes; }
	inline const NodesContainer & nodes() const { return m_d->nodes; }
	inline NodePtrContainer & nodePtrs() { return m_d->nodePtrs;}
	inline const NodePtrContainer & nodePtrs() const { return m_d->nodePtrs;}
	inline CellsContainer & cells() { return m_d->cells; }
	inline const CellsContainer & cells() const { return m_d->cells; }
	inline std::size_t size() const { return nodes().size()-1;}
	inline const Node & at(uint32_t pos) const { return nodes().at(pos); }
	inline NodeChilrenIterator childrenBegin(uint32_t nodePos) const { return nodePtrs().cbegin() + at(nodePos).m_childrenBegin; }
	inline NodeChilrenIterator childrenEnd(uint32_t nodePos) const { return nodePtrs().cbegin() + at(nodePos+1).m_childrenBegin; }
	inline CellsIterator cellsBegin(uint32_t nodePos) const { return cells().cbegin() + at(nodePos).m_cellsBegin;}
	inline CellsIterator cellsEnd(uint32_t nodePos) const { return cells().cbegin() + at(nodePos+1).m_cellsBegin;}
};

class GeoHierarchy: public RefCountObject {
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef sserialize::Static::Array<uint32_t> StoreIdToGhIdMap;
	typedef sserialize::MultiVarBitArray RegionDescriptionType;
	typedef sserialize::BoundedCompactUintArray RegionPtrListType;
	
	typedef sserialize::MultiVarBitArray CellDescriptionType;
	typedef sserialize::BoundedCompactUintArray CellPtrListType;
private:
	SubSetBase::Node * createSubSet(const CellQueryResult & cqr, SubSetBase::Node** nodes, uint32_t size, uint32_t threadCount) const;
	SubSetBase::Node * createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSetBase::Node*> & nodes) const;
private:
	StoreIdToGhIdMap m_storeIdToGhId;
	RegionDescriptionType m_regions;
	RegionPtrListType m_regionPtrs;
	sserialize::Static::Array<sserialize::spatial::GeoRect> m_regionBoundaries;
	CellDescriptionType m_cells;
	CellPtrListType m_cellPtrs;
	sserialize::Static::Array<sserialize::spatial::GeoRect> m_cellBoundaries;
public:
	GeoHierarchy();
	GeoHierarchy(const GeoHierarchy & other) = delete;
	GeoHierarchy(const UByteArrayAdapter & data);
	virtual ~GeoHierarchy();
	OffsetType getSizeInBytes() const;
	
	bool hasRegionItems() const;
	bool hasRegionNeighbors() const;

	const RegionDescriptionType & regions() const { return m_regions; }
	const CellDescriptionType & cells() const { return m_cells; }
	const RegionPtrListType & regionPtrs() const { return m_regionPtrs; }
	///Points of cells, these are the cell parents
	const CellPtrListType & cellPtrs() const { return m_cellPtrs; }

	uint32_t cellSize() const;
	uint32_t cellParentsBegin(uint32_t id) const;
	uint32_t cellDirectParentsEnd(uint32_t id) const;
	uint32_t cellParentsEnd(uint32_t id) const;
	uint32_t cellParentsSize(uint32_t id) const;
	uint32_t cellPtrsSize() const;
	uint32_t cellPtr(uint32_t pos) const;
	uint32_t cellItemsPtr(uint32_t pos) const;
	uint32_t cellItemsCount(uint32_t pos) const;
	
	uint32_t regionSize() const;
	uint32_t storeIdToGhId(uint32_t storeId) const;
	uint32_t ghIdToStoreId(uint32_t regionId) const;
	bool regionHasItemsInfo(uint32_t regionId) const;
	
	uint32_t regionCellIdxPtr(uint32_t pos) const;
	uint32_t regionExclusiveCellIdxPtr(uint32_t pos) const;
	uint32_t regionItemsPtr(uint32_t pos) const;
	uint32_t regionItemsCount(uint32_t pos) const;
	uint32_t regionCellSumItemsCount(uint32_t pos) const;
	
	uint32_t regionChildrenBegin(uint32_t id) const;
	uint32_t regionChildrenEnd(uint32_t id) const;
	uint32_t regionChildrenSize(uint32_t id) const;
	
	uint32_t regionParentsBegin(uint32_t id) const;
	uint32_t regionParentsEnd(uint32_t id) const;
	uint32_t regionParentsSize(uint32_t id) const;
	
	uint32_t regionNeighborsBegin(uint32_t id) const;
	uint32_t regionNeighborsEnd(uint32_t id) const;
	uint32_t regionNeighborsSize(uint32_t id) const;
	
	uint32_t regionPtrSize() const;
	uint32_t regionPtr(uint32_t pos) const;
	
	sserialize::spatial::GeoRect regionBoundary(uint32_t id) const;
	sserialize::spatial::GeoRect cellBoundary(uint32_t id) const;
	
	SubSet subSet(const sserialize::CellQueryResult& cqr, bool sparse, uint32_t threadCount) const;
	FlatSubSet flatSubSet(const sserialize::CellQueryResult& cqr, bool sparse) const;
};

class Cell {
public:
	typedef enum {CD_ITEM_PTR=0, CD_ITEM_COUNT=1, CD_PARENTS_BEGIN=2, CD_DIRECT_PARENTS_OFFSET=3, CD__ENTRY_SIZE=4} CellDescriptionAccessors;
private:
	uint32_t m_pos;
	RCPtrWrapper<GeoHierarchy> m_db;
public:
	Cell();
	Cell(uint32_t pos, const RCPtrWrapper<GeoHierarchy> & db);
	virtual ~Cell();
	inline uint32_t ghId() { return m_pos; }
	uint32_t itemPtr() const;
	uint32_t itemCount() const;
	///Offset into PtrArray
	uint32_t parentsBegin() const;
	///Offset into PtrArray
	uint32_t directParentsEnd() const;
	///Offset into PtrArray
	uint32_t parentsEnd() const;
	uint32_t parentsSize() const;
	///@return pos of the parent
	uint32_t parent(uint32_t pos) const;
	sserialize::spatial::GeoRect boundary() const;
};

class Region {
public:
	typedef enum {RD_CELL_LIST_PTR=0, RD_EXCLUSIVE_CELL_LIST_PTR=1, RD_TYPE=2, RD_STORE_ID=3,
					RD_CHILDREN_BEGIN=4, RD_PARENTS_OFFSET=5, RD_NEIGHBORS_OFFSET=6,
					RD_ITEMS_PTR=7, RD_ITEMS_COUNT=8,
					RD__ENTRY_SIZE=9} RegionDescriptionAccessors;
private:
	uint32_t m_pos;
	RCPtrWrapper<GeoHierarchy> m_db;
public:
	Region();
	///Only to be used data with getPtr = 0
	Region(uint32_t pos, const RCPtrWrapper<GeoHierarchy> & db);
	virtual ~Region();
	sserialize::spatial::GeoShapeType type() const;
	inline uint32_t ghId() const { return m_pos; }
	uint32_t storeId() const;
	sserialize::spatial::GeoRect boundary() const;
	bool hasItemsInfo() const;
	uint32_t cellIndexPtr() const;
	uint32_t exclusiveCellIndexPtr() const;
	///Offset into PtrArray
	uint32_t parentsBegin() const;
	///Offset into PtrArray
	uint32_t parentsEnd() const;
	uint32_t parentsSize() const;
	///@return id of the parent
	uint32_t parent(uint32_t pos) const;
	
	///Offset into PtrArray
	uint32_t neighborsBegin() const;
	///Offset into PtrArray
	uint32_t neighborsEnd() const;
	uint32_t neighborsSize() const;
	///@return id of the neighbor
	uint32_t neighbor(uint32_t pos) const;

	uint32_t childrenSize() const;
	///Offset into PtrArray
	uint32_t childrenBegin() const;
	///Offset into PtrArray
	uint32_t childrenEnd() const;
	uint32_t child(uint32_t pos) const;
	
	///return the items in this region
	uint32_t itemsPtr() const;
	uint32_t itemsCount() const;
};

}//end namespace detail

class GeoHierarchy {
private:
	friend class detail::GeoHierarchy;
private:
	typedef RCPtrWrapper<detail::GeoHierarchy> DataPtr;
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef detail::GeoHierarchy::RegionDescriptionType RegionDescriptionType;
	typedef detail::GeoHierarchy::RegionPtrListType RegionPtrListType;
	
	typedef detail::GeoHierarchy::CellDescriptionType CellDescriptionType;
	typedef detail::GeoHierarchy::CellPtrListType CellPtrListType;

	typedef detail::Region Region;
	typedef detail::Cell Cell;
	typedef detail::SubSet SubSet;
	typedef detail::FlatSubSet FlatSubSet;
private:
	RCPtrWrapper<detail::GeoHierarchy> m_priv;
public:
	GeoHierarchy() : m_priv(new detail::GeoHierarchy()) {}
	GeoHierarchy(const UByteArrayAdapter & data) : m_priv(new detail::GeoHierarchy(data)) {}
	GeoHierarchy(const GeoHierarchy & other) : m_priv(other.m_priv) {}
	virtual ~GeoHierarchy() {}
	GeoHierarchy & operator=(GeoHierarchy const &) = default;
	inline OffsetType getSizeInBytes() const { return m_priv->getSizeInBytes(); }
	
	bool hasRegionItems() const { return m_priv->hasRegionItems(); }
	bool hasRegionNeighbors() const { return m_priv->hasRegionNeighbors(); }
	
	inline const RegionDescriptionType & regions() const { return m_priv->regions(); }
	inline const CellDescriptionType & cells() const { return m_priv->cells(); }
	inline const CellPtrListType & cellPtrs() const { return m_priv->cellPtrs(); }
	inline const RegionPtrListType & regionPtrs() const { return m_priv->regionPtrs(); }
	
	inline uint32_t cellSize() const { return m_priv->cellSize(); }
	Cell cell(uint32_t id) const;
	
	inline uint32_t cellParentsBegin(uint32_t id) const { return m_priv->cellParentsBegin(id);}
	inline uint32_t cellDirectParentsEnd(uint32_t id) const { return m_priv->cellDirectParentsEnd(id);}
	inline uint32_t cellParentsEnd(uint32_t id) const { return m_priv->cellParentsEnd(id);}
	inline uint32_t cellParentsSize(uint32_t id) const { return m_priv->cellParentsSize(id); }
	inline uint32_t cellPtrsSize() const { return m_priv->cellPtrsSize();}
	inline uint32_t cellPtr(uint32_t pos) const { return m_priv->cellPtr(pos);}
	inline uint32_t cellItemsPtr(uint32_t pos) const { return m_priv->cellItemsPtr(pos);}
	inline uint32_t cellItemsCount(uint32_t pos) const { return m_priv->cellItemsCount(pos);}
	
	inline uint32_t regionSize() const { return m_priv->regionSize();}
	inline uint32_t storeIdToGhId(uint32_t storeId) const { return m_priv->storeIdToGhId(storeId);}
	inline uint32_t ghIdToStoreId(uint32_t regionId) const { return m_priv->ghIdToStoreId(regionId); }
	
	Region region(uint32_t id) const;
	Region rootRegion() const;
	inline Region regionFromStoreId(uint32_t storeId) const { return region(storeIdToGhId(storeId)); }
	
	inline uint32_t regionItemsPtr(uint32_t pos) const { return m_priv->regionItemsPtr(pos);}
	inline uint32_t regionItemsCount(uint32_t pos) const { return m_priv->regionItemsCount(pos);}
	inline uint32_t regionCellSumItemsCount(uint32_t pos) const { return m_priv->regionCellSumItemsCount(pos);}
	
	inline uint32_t regionCellIdxPtr(uint32_t id) const { return m_priv->regionCellIdxPtr(id); }
	///These are the cells that are only part of region_id and thus not part of a child
	inline uint32_t regionExclusiveCellIdxPtr(uint32_t id) const { return m_priv->regionExclusiveCellIdxPtr(id); }
	
	inline uint32_t regionChildrenBegin(uint32_t id) const { return m_priv->regionChildrenBegin(id);}
	inline uint32_t regionChildrenEnd(uint32_t id) const { return m_priv->regionChildrenEnd(id); }
	
	inline uint32_t regionParentsBegin(uint32_t id) const { return m_priv->regionParentsBegin(id);}
	inline uint32_t regionParentsEnd(uint32_t id) const { return m_priv->regionParentsEnd(id); }
	inline uint32_t regionPtrSize() const { return m_priv->regionPtrSize();}
	inline uint32_t regionPtr(uint32_t pos) const { return m_priv->regionPtr(pos);}
	
	inline sserialize::spatial::GeoRect regionBoundary(uint32_t id) const { return m_priv->regionBoundary(id);}
	inline sserialize::spatial::GeoRect cellBoundary(uint32_t id) const { return m_priv->cellBoundary(id);}
	
	bool consistencyCheck(const sserialize::Static::ItemIndexStore& store) const;
	
	std::ostream & printStats(std::ostream & out, const sserialize::Static::ItemIndexStore & store) const;
	
	///@return cells whose bbox intersects @param rect
	sserialize::ItemIndex intersectingCells(const sserialize::Static::ItemIndexStore& idxStore, const sserialize::spatial::GeoRect & rect, uint32_t threadCount = 1) const;

	SubSet subSet(const sserialize::CellQueryResult & cqr, bool sparse, uint32_t threadCount) const;
	FlatSubSet flatSubSet(const sserialize::CellQueryResult & cqr, bool sparse) const;
private:
	GeoHierarchy(const RCPtrWrapper<detail::GeoHierarchy> & p) : m_priv(p) {}
};

class GeoHierarchyCellInfo: public interface::CQRCellInfoIface {
public:
	GeoHierarchyCellInfo(const sserialize::Static::spatial::GeoHierarchy & gh);
	virtual ~GeoHierarchyCellInfo() override {}
	
	virtual SizeType cellSize() const override;
	virtual sserialize::spatial::GeoRect cellBoundary(CellId cellId) const override;
	virtual SizeType cellItemsCount(CellId cellId) const override;
	virtual IndexId cellItemsPtr(CellId cellId) const override;
public:
	static sserialize::RCPtrWrapper<interface::CQRCellInfoIface> makeRc(const sserialize::Static::spatial::GeoHierarchy & gh);
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
};

namespace detail {

class SubSet: public SubSetBase {
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	NodePtr m_root;
	CellQueryResult m_cqr;
	bool m_sparse;
private:
	template<typename T_HASH_CONTAINER>
	void insertCellPositions(const NodePtr & node, T_HASH_CONTAINER & idcsPos) const;
	bool regionByGhId(const NodePtr& node, uint32_t ghId, NodePtr & dest) const;
public:
	SubSet() {}
	SubSet(Node * root, const sserialize::Static::spatial::GeoHierarchy & gh, const CellQueryResult & cqr, bool sparse);
	virtual ~SubSet()  {}
	inline const NodePtr & root() const { return m_root;}
	inline const CellQueryResult & cqr() const { return m_cqr; }
	const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const;
	///Sparse SubSets have no itemcounts and need a recursive flattening strategy
	inline bool sparse() const { return m_sparse; }
	sserialize::ItemIndex items(const NodePtr & node) const;
	sserialize::ItemIndex cells(const NodePtr & node) const;
	sserialize::ItemIndex topK(const NodePtr & node, uint32_t numItems) const;
	uint32_t storeId(const NodePtr & node) const;
	NodePtr regionByStoreId(uint32_t storeId) const;
	///Get a path to the first region where the result set branches into different regions
	///@param fraction path ends if less than @fraction elements are within the next region
	///@param globalFraction calculate fraction relative to all results or relative to the local region
	///@param out : operator()(const NodePtr & node);
	template<typename TOutputIterator>
	void pathToBranch(TOutputIterator out, double fraction = 0.95, bool globalFraction = true) const;
};


template<typename T_HASH_CONTAINER>
void SubSet::insertCellPositions(const NodePtr & node, T_HASH_CONTAINER & idcsPos) const {
	idcsPos.insert(node->cellPositions().cbegin(), node->cellPositions().cend());
	for(uint32_t i(0), s((uint32_t) node->size()); i < s; ++i) {
		insertCellPositions(node->at(i), idcsPos);
	}
}

template<typename TOutputIterator>
void SubSet::pathToBranch(TOutputIterator out, double fraction, bool globalFraction) const {
	typedef Node::iterator NodeIterator;
	NodePtr rPtr = root();
	double referenceItemCount = rPtr->maxItemsSize();
	uint32_t curMax = 0;
	while (rPtr->size()) {
		curMax = 0;
		NodeIterator curMaxChild = rPtr->begin();
		for(NodeIterator it(rPtr->begin()), end(rPtr->end()); it != end; ++it) {
			uint32_t tmp = (*it)->maxItemsSize();
			if ( tmp > curMax ) {
				curMax = tmp;
				curMaxChild = it;
			}
		}
		if ((double)(curMax)/referenceItemCount >= fraction) {
			rPtr = *curMaxChild;
			*out = rPtr;
			++out;
			if (!globalFraction) {
				referenceItemCount = curMax;
			}
		}
		else {
			break;
		}
	}
}

	
} //end namespace detail

}}} //end namespace sserialize::Static::spatial

std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Cell & r);
std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Region & r);

#endif
