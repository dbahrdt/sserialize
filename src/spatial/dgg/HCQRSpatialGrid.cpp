#include <sserialize/spatial/dgg/HCQRSpatialGrid.h>

#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/spatial/CellQueryResult.h>

#include <memory>

namespace sserialize::spatial::dgg::impl::detail::HCQRSpatialGrid {

TreeNode::TreeNode(PixelId pixelId, int flags, uint32_t itemIndexId) :
m_pid(pixelId),
m_f(flags),
m_itemIndexId(itemIndexId)
{}

std::unique_ptr<TreeNode>
TreeNode::make_unique(PixelId pixelId, int flags, uint32_t itemIndexId) {
	SSERIALIZE_CHEAP_ASSERT(flags == IS_INTERNAL || itemIndexId != std::numeric_limits<uint32_t>::max() || flags == IS_FULL_MATCH);
	return std::unique_ptr<TreeNode>( new TreeNode(pixelId, flags, itemIndexId) );
}

std::unique_ptr<TreeNode>
TreeNode::shallowCopy() const {
    SSERIALIZE_CHEAP_ASSERT(!isFetched());
    return TreeNode::make_unique(pixelId(), m_f, m_itemIndexId);
}

std::unique_ptr<TreeNode>
TreeNode::shallowCopy(uint32_t fetchedItemIndexId) const {
    SSERIALIZE_CHEAP_ASSERT(isFetched());
    return TreeNode::make_unique(pixelId(), m_f, fetchedItemIndexId);
}

std::unique_ptr<TreeNode>
TreeNode::shallowCopy(Children && newChildren) const {
    SSERIALIZE_CHEAP_ASSERT(isInternal())
    auto result = TreeNode::make_unique(pixelId(), m_f);
    result->children() = std::move(newChildren);
    return result;
}

void TreeNode::sortChildren() {
	sort(children().begin(), children().end(),
		[](std::unique_ptr<TreeNode> const & a, std::unique_ptr<TreeNode> const & b) -> bool {
			return a->pixelId() < b->pixelId();
		}
	);
}

bool TreeNode::valid() const {
	return (flags() == IS_INTERNAL && children().size()) || (flags() == IS_FULL_MATCH && itemIndexId() == npos) || (flags() == IS_FETCHED && itemIndexId() != npos) || (flags() == IS_PARTIAL_MATCH && itemIndexId() != npos);
}

}//end namespace sserialize::spatial::dgg::impl::detail::HCQRSpatialGrid

namespace sserialize::spatial::dgg::impl {

HCQRSpatialGrid::HCQRSpatialGrid(
	sserialize::Static::ItemIndexStore idxStore,
	sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
	sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi
) :
Parent(sg, sgi),
m_items(idxStore)
{}

HCQRSpatialGrid::HCQRSpatialGrid(
	sserialize::CellQueryResult const & cqr,
    sserialize::Static::ItemIndexStore idxStore,
    sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
    sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi
) :
HCQRSpatialGrid(idxStore, sg, sgi)
{
    std::unordered_map<PixelId, std::unique_ptr<TreeNode>> clevel;
    clevel.reserve(cqr.cellCount());
    for(auto it(cqr.begin()), end(cqr.end()); it != end; ++it) {
		if (it.fullMatch()) {
			PixelId pId = this->sgi().pixelId( CompressedPixelId(it.cellId()) );
			clevel[pId] = TreeNode::make_unique(pId, TreeNode::IS_FULL_MATCH);
		}
		else if (it.fetched()) {
			PixelId pId = this->sgi().pixelId( CompressedPixelId(it.cellId()) );
			m_fetchedItems.push_back( it.idx() );
			clevel[pId] = TreeNode::make_unique(pId, TreeNode::IS_FETCHED, m_fetchedItems.size());
		}
		else {
			PixelId pId = this->sgi().pixelId( CompressedPixelId(it.cellId()) );
			clevel[pId] = TreeNode::make_unique(pId, TreeNode::IS_PARTIAL_MATCH, it.idxId());
		}
    }
    while (clevel.size() > 1 || (clevel.size() && this->sg().level( clevel.begin()->second->pixelId() ) > 0)) {
		SSERIALIZE_EXPENSIVE_ASSERT_EXEC(auto clvl = this->sg().level( clevel.begin()->second->pixelId() ); );
        std::unordered_map<PixelId, std::unique_ptr<TreeNode>> plevel;
        for(auto & x : clevel) {
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(clvl, this->sg().level( x.second->pixelId() ));
            PixelId pPId = this->sg().parent( x.second->pixelId() );
            auto & parent = plevel[pPId];
            if (!parent) {
                parent = TreeNode::make_unique(pPId, TreeNode::IS_INTERNAL);
            }
            parent->children().emplace_back( std::move(x.second) );
        }
        clevel = std::move(plevel);
        ///children have to be sorted according to their PixelId
        for(auto & x : clevel) {
            using std::sort;
			x.second->sortChildren();
        }
    }
    if (clevel.size()) {
		m_root = std::move(clevel.begin()->second);
	}
}

HCQRSpatialGrid::HCQRSpatialGrid(
	TreeNodePtr && root,
	sserialize::Static::ItemIndexStore idxStore,
	sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
	sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi
) :
Parent(sg, sgi),
m_root(std::move(root)),
m_items(idxStore)
{}

HCQRSpatialGrid::~HCQRSpatialGrid() {}

HCQRSpatialGrid::SizeType
HCQRSpatialGrid::depth() const {
    struct Recurser {
        SizeType operator()(TreeNode & node) const {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (node.children().size()) {
                SizeType tmp = 0;
                for(auto const & x : node.children()) {
                    tmp = std::max(tmp, (*this)(*x));
                }
                return tmp+1;
            }
            else {
                return 0;
            }
        }
    };
	if (m_root) {
		return Recurser()(*m_root);
	}
	else {
		return 0;
	}
}

HCQRSpatialGrid::SizeType
HCQRSpatialGrid::numberOfItems() const {
    struct Recurser {
        HCQRSpatialGrid const & that;
        SizeType numberOfItems{0};
        void operator()(TreeNode const & node) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (node.isInternal()) {
                for(auto const & x : node.children()) {
                    (*this)(*x);
                }
            }
            else if (node.isFullMatch()) {
                numberOfItems += that.sgi().itemCount(node.pixelId());
            }
            else if (node.isFetched()) {
                numberOfItems += that.fetchedItems().at(node.itemIndexId()).size();
            }
            else { //partial-match
                numberOfItems += that.idxStore().idxSize(node.itemIndexId());
            }
        }
        Recurser(HCQRSpatialGrid const & that) : that(that) {}
    };
	if (m_root) {
		Recurser r(*this);
		r(*m_root);
		return r.numberOfItems;
	}
	else {
		return 0;
	}
}

HCQRSpatialGrid::SizeType
HCQRSpatialGrid::numberOfNodes() const {
    struct Recurser {
        HCQRSpatialGrid const & that;
        SizeType numberOfNodes{0};
        void operator()(TreeNode const & node) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
			numberOfNodes += 1;
            if (node.isInternal()) {
                for(auto const & x : node.children()) {
                    (*this)(*x);
                }
            }
        }
        Recurser(HCQRSpatialGrid const & that) : that(that) {}
    };
	if (m_root) {
		Recurser r(*this);
		r(*m_root);
		return r.numberOfNodes;
	}
	else {
		return 0;
	}
}

HCQRSpatialGrid::ItemIndex
HCQRSpatialGrid::items() const {
	if (m_root) {
		return items(*m_root);;
	}
	else {
		return ItemIndex();
	}
}

struct HCQRSpatialGrid::HCQRSpatialGridOpHelper {
    HCQRSpatialGrid & dest;
    HCQRSpatialGridOpHelper(HCQRSpatialGrid & dest) : dest(dest) {}
    std::unique_ptr<HCQRSpatialGrid::TreeNode> deepCopy(HCQRSpatialGrid const & src, HCQRSpatialGrid::TreeNode const & node) {
		SSERIALIZE_NORMAL_ASSERT(node.valid());
		if (node.isInternal()) {
			auto result = node.shallowCopy();
			for(auto const & x : node.children()) {
				result->children().emplace_back( this->deepCopy(src, *x) );
			}
			return result;
		}
        else if (node.isFetched()) {
            dest.m_fetchedItems.emplace_back( src.items(node) );
            return node.shallowCopy(dest.m_fetchedItems.size()-1);
        }
        else {
            return node.shallowCopy();
        }
    }

    PixelId resultPixelId(HCQRSpatialGrid::TreeNode const & first, HCQRSpatialGrid::TreeNode const & second) const {
		SSERIALIZE_NORMAL_ASSERT(first.valid());
		SSERIALIZE_NORMAL_ASSERT(second.valid());
        if (first.pixelId() == second.pixelId()) {
            return first.pixelId();
        }
        else if (dest.sg().isAncestor(first.pixelId(), second.pixelId())) {
            return second.pixelId();
        }
        else if (dest.sg().isAncestor(second.pixelId(), first.pixelId())) {
            return first.pixelId();
        }
        else {
            throw sserialize::BugException("Trying to compute common node for non-related tree nodes");
        }
    }

    void sortChildren(TreeNode & node) {
		SSERIALIZE_NORMAL_ASSERT(node.valid());
        sort(node.children().begin(), node.children().end(), [](std::unique_ptr<TreeNode> const & a, std::unique_ptr<TreeNode> const & b) {
            return a->pixelId() < b->pixelId();
        });
    }
};

HCQRSpatialGrid::HCQRPtr
HCQRSpatialGrid::operator/(HCQR const & other) const {
	if (!dynamic_cast<Self const *>(&other)) {
		throw sserialize::TypeMissMatchException("Incorrect input type");
	}
    struct Recurser: public HCQRSpatialGridOpHelper {
        HCQRSpatialGrid const & firstSg;
        HCQRSpatialGrid const & secondSg;
        Recurser(HCQRSpatialGrid const & firstSg, HCQRSpatialGrid const & secondSg, HCQRSpatialGrid & dest) :
        HCQRSpatialGridOpHelper(dest),
        firstSg(firstSg),
        secondSg(secondSg)
        {}
        std::unique_ptr<TreeNode> operator()(TreeNode const & firstNode, TreeNode const & secondNode) {
			SSERIALIZE_NORMAL_ASSERT(firstNode.valid());
			SSERIALIZE_NORMAL_ASSERT(secondNode.valid());
			std::unique_ptr<TreeNode> rptr;
			if (firstNode.isFullMatch() && secondNode.isFullMatch()) {
				SSERIALIZE_CHEAP_ASSERT_EQUAL(firstSg.sg().level(firstNode.pixelId()), secondSg.sg().level(secondNode.pixelId()));
				rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_FULL_MATCH);
			}
            else if (firstNode.isFullMatch() && secondNode.isInternal()) {
				SSERIALIZE_CHEAP_ASSERT_EQUAL(firstSg.sg().level(firstNode.pixelId()), secondSg.sg().level(secondNode.pixelId()));
				rptr = deepCopy(secondSg, secondNode);
            }
            else if (secondNode.isFullMatch() && firstNode.isInternal()) {
				SSERIALIZE_CHEAP_ASSERT_EQUAL(firstSg.sg().level(firstNode.pixelId()), secondSg.sg().level(secondNode.pixelId()));
                rptr = deepCopy(firstSg, firstNode);
            }
            else if (firstNode.isLeaf() && secondNode.isLeaf()) {
                auto result = firstSg.items(firstNode) / secondSg.items(secondNode);
                if (!result.size()) {
                    return std::unique_ptr<TreeNode>();
                }
                dest.m_fetchedItems.emplace_back(result);
                rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
            }
            else {
                rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_INTERNAL);

                if (firstNode.isInternal() && secondNode.isInternal()) {
                    auto fIt = firstNode.children().begin();
                    auto fEnd = firstNode.children().end();
                    auto sIt = secondNode.children().begin();
                    auto sEnd = secondNode.children().end();
                    for(;fIt != fEnd && sIt != sEnd;) {
                        if ((*fIt)->pixelId() < (*sIt)->pixelId()) {
                            ++fIt;
                        }
                        else if ((*fIt)->pixelId() > (*sIt)->pixelId()) {
                            ++sIt;
                        }
                        else {
                            auto x = (*this)(**fIt, **sIt);
                            if (x) {
                                rptr->children().emplace_back(std::move(x));
                            }
                            ++fIt;
							++sIt;
                        }
                    }
                }
                else if (firstNode.isInternal()) {
                    auto fIt = firstNode.children().begin();
                    auto fEnd = firstNode.children().end();
                    for( ;fIt != fEnd; ++fIt) {
                        auto x = (*this)(**fIt, secondNode);
                        if (x) {
                            rptr->children().emplace_back(std::move(x));
                        }
                    }
                }
                else if (secondNode.isInternal()) {
                    auto sIt = secondNode.children().begin();
                    auto sEnd = secondNode.children().end();
                    for( ;sIt != sEnd; ++sIt) {
                        auto x = (*this)(firstNode, **sIt);
                        if (x) {
                            rptr->children().emplace_back(std::move(x));
                        }
                    }
                }

                if (!rptr->children().size()) {
                    rptr = std::unique_ptr<TreeNode>();
                }
            }
			#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
				if (rptr) {
					if (firstSg.items(firstNode) / secondSg.items(secondNode) != dest.items(*rptr)) {
						auto sdiff = (firstSg.items(firstNode) / secondSg.items(secondNode)) ^ dest.items(*rptr);
						for(auto x : sdiff) {
							for (auto const & c : rptr->children()) {
								if (dest.items(*c).count(x)) {
									std::cout << "item:" << x << " -> " << "pixel:" << c->pixelId() << std::endl;  
								}
							}
						}
					}
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(firstSg.items(firstNode) / secondSg.items(secondNode), dest.items(*rptr));
				}
				else {
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(0, (firstSg.items(firstNode) / secondSg.items(secondNode)).size());
				}
			#endif
			return rptr;
        }
    };
    sserialize::RCPtrWrapper<Self> dest( new Self(m_items, sgPtr(), sgiPtr()) );
	if (m_root && static_cast<Self const &>(other).m_root) {
		Recurser rec(*this, static_cast<Self const &>(other), *dest);
		dest->m_root = rec(*(this->m_root), *(static_cast<Self const &>(other).m_root));
	}
	SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(items() / other.items(), dest->items());
    return dest;
}

HCQRSpatialGrid::HCQRPtr
HCQRSpatialGrid::operator+(HCQR const & other) const {
	if (!dynamic_cast<Self const *>(&other)) {
		throw sserialize::TypeMissMatchException("Incorrect input type");
	}
    struct Recurser: public HCQRSpatialGridOpHelper {
        HCQRSpatialGrid const & firstSg;
        HCQRSpatialGrid const & secondSg;
        Recurser(HCQRSpatialGrid const & firstSg, HCQRSpatialGrid const & secondSg, HCQRSpatialGrid & dest) :
        HCQRSpatialGridOpHelper(dest),
        firstSg(firstSg),
        secondSg(secondSg)
        {}
        TreeNodePtr operator()(TreeNode const & firstNode, TreeNode const & secondNode) {
			SSERIALIZE_NORMAL_ASSERT(firstNode.valid());
			SSERIALIZE_NORMAL_ASSERT(secondNode.valid());
			TreeNodePtr rptr;
			if (firstNode.isFullMatch()) {
				rptr = TreeNode::make_unique(firstNode.pixelId(), TreeNode::IS_FULL_MATCH);
			}
            else if (secondNode.isFullMatch()) {
				rptr = TreeNode::make_unique(secondNode.pixelId(), TreeNode::IS_FULL_MATCH);
            }
            else if (firstNode.isLeaf() && secondNode.isLeaf()) {
				auto fnLvl = firstSg.level(firstNode);
				auto snLvl = secondSg.level(secondNode);
				sserialize::ItemIndex result;
				if (fnLvl == snLvl) {
					result = firstSg.items(firstNode) + secondSg.items(secondNode);
				}
				else if (fnLvl < snLvl) { //firstNode is an ancestor of secondNode
					SSERIALIZE_NORMAL_ASSERT(firstSg.sg().isAncestor(firstNode.pixelId(), secondNode.pixelId()));
					result = (firstSg.items(firstNode) / firstSg.sgi().items(secondNode.pixelId())) + secondSg.items(secondNode);
				}
				else if (snLvl < fnLvl) { //secondNode is an ancestor of firstNode
					SSERIALIZE_NORMAL_ASSERT(firstSg.sg().isAncestor(secondNode.pixelId(), firstNode.pixelId()));
					result = firstSg.items(firstNode) + (secondSg.items(secondNode) / secondSg.sgi().items(firstNode.pixelId()));
				}
				SSERIALIZE_CHEAP_ASSERT(result.size());
                dest.m_fetchedItems.emplace_back(result);
                rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
            }
            else {
                rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_INTERNAL);

                if (firstNode.isInternal() && secondNode.isInternal()) {
                    auto fIt = firstNode.children().begin();
                    auto fEnd = firstNode.children().end();
                    auto sIt = secondNode.children().begin();
                    auto sEnd = secondNode.children().end();
                    for(;fIt != fEnd && sIt != sEnd;) {
                        if ((*fIt)->pixelId() < (*sIt)->pixelId()) {
							rptr->children().emplace_back(deepCopy(firstSg, **fIt));
                            ++fIt;
                        }
                        else if ((*fIt)->pixelId() > (*sIt)->pixelId()) {
							rptr->children().emplace_back( deepCopy(secondSg, **sIt) );
                            ++sIt;
                        }
                        else {
							rptr->children().emplace_back((*this)(**fIt, **sIt));
                            ++fIt;
							++sIt;
                        }
                    }
                    for(; fIt != fEnd; ++fIt) {
						rptr->children().emplace_back( deepCopy(firstSg, **fIt) );
					}
					for(; sIt != sEnd; ++sIt) {
						rptr->children().emplace_back( deepCopy(secondSg, **sIt) );
					}
                }
                else if (firstNode.isInternal()) {
					SSERIALIZE_CHEAP_ASSERT(!secondNode.isFullMatch() && secondNode.isLeaf());
					std::vector<PixelId> virtSecondPids = secondSg.pixelChildren(firstNode.pixelId());
					
                    auto fIt = firstNode.children().begin();
                    auto fEnd = firstNode.children().end();
					auto sIt = virtSecondPids.begin();
					auto sEnd = virtSecondPids.end();
					auto secondNodeItems = secondSg.items(secondNode);
                    for(;fIt != fEnd && sIt != sEnd; ++sIt) {
						TreeNodePtr x;
						if (*sIt < (*fIt)->pixelId()) {
							auto result = secondNodeItems / secondSg.sgi().items(*sIt);
							if (result.size()) {
								dest.m_fetchedItems.emplace_back(result);
								x = TreeNode::make_unique(*sIt, TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
							}
						}
						else {
							SSERIALIZE_ASSERT_EQUAL((*fIt)->pixelId(), *sIt);
							x = (*this)(**fIt, secondNode);
							++fIt;
						}
                        if (x) {
                            rptr->children().emplace_back(std::move(x));
                        }
                    }
                    for(; sIt != sEnd; ++sIt) {
						auto result = secondNodeItems / secondSg.sgi().items(*sIt);
						if (result.size()) {
							dest.m_fetchedItems.emplace_back(result);
							rptr->children().emplace_back( TreeNode::make_unique(*sIt, TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1) );
						}
					}
                }
                else if (secondNode.isInternal()) {
					SSERIALIZE_CHEAP_ASSERT(!firstNode.isFullMatch() && firstNode.isLeaf());
					std::vector<PixelId> virtFirstPids = firstSg.pixelChildren(secondNode.pixelId());
					
                    auto sIt = secondNode.children().begin();
                    auto sEnd = secondNode.children().end();
					auto fIt = virtFirstPids.begin();
					auto fEnd = virtFirstPids.end();
					auto firstNodeItems = firstSg.items(firstNode);
                    for(;fIt != fEnd && sIt != sEnd; ++fIt) {
						TreeNodePtr x;
						if (*fIt < (*sIt)->pixelId()) {
							auto result = firstNodeItems / firstSg.sgi().items(*fIt);
							if (result.size()) {
								dest.m_fetchedItems.emplace_back(result);
								x = TreeNode::make_unique(*fIt, TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
							}
						}
						else {
							SSERIALIZE_ASSERT_EQUAL(*fIt, (*sIt)->pixelId());
							x = (*this)(firstNode, **sIt);
							++sIt;
						}
                        if (x) {
                            rptr->children().emplace_back(std::move(x));
                        }
                    }
                    for(; fIt != fEnd; ++fIt) {
						auto result = firstNodeItems / firstSg.sgi().items(*fIt);
						if (result.size()) {
							dest.m_fetchedItems.emplace_back(result);
							rptr->children().emplace_back( TreeNode::make_unique(*fIt, TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1) );
						}
					}
                }
                SSERIALIZE_CHEAP_ASSERT(rptr->children().size());
            }
            SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(firstSg.items(firstNode) + secondSg.items(secondNode), dest.items(*rptr));
            return rptr;
        }
    };
    sserialize::RCPtrWrapper<Self> dest( new Self(m_items, sgPtr(), sgiPtr()) );
	if (m_root && static_cast<Self const &>(other).m_root) {
		Recurser rec(*this, static_cast<Self const &>(other), *dest);
		dest->m_root = rec(*(this->m_root), *(static_cast<Self const &>(other).m_root));
	}
	SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(items() + other.items(), dest->items());
    return dest;
}

HCQRSpatialGrid::HCQRPtr
HCQRSpatialGrid::operator-(HCQR const & other) const {
	if (!dynamic_cast<Self const *>(&other)) {
		throw sserialize::TypeMissMatchException("Incorrect input type");
	}
    struct Recurser: public HCQRSpatialGridOpHelper {
        HCQRSpatialGrid const & firstSg;
        HCQRSpatialGrid const & secondSg;
        Recurser(HCQRSpatialGrid const & firstSg, HCQRSpatialGrid const & secondSg, HCQRSpatialGrid & dest) :
        HCQRSpatialGridOpHelper(dest),
        firstSg(firstSg),
        secondSg(secondSg)
        {}
        auto level(HCQRSpatialGrid const & sg, TreeNode const & node) {
			return sg.sg().level(node.pixelId());
		}
        std::unique_ptr<TreeNode> operator()(TreeNode const & firstNode, TreeNode const & secondNode) {
			SSERIALIZE_NORMAL_ASSERT(firstNode.valid());
			SSERIALIZE_NORMAL_ASSERT(secondNode.valid());
			TreeNodePtr rptr;
			if (secondNode.isFullMatch() && level(firstSg, firstNode) >= level(secondSg, secondNode)) {
				SSERIALIZE_CHEAP_ASSERT_EQUAL(level(firstSg, firstNode), level(secondSg, secondNode));
				rptr = std::unique_ptr<TreeNode>();
			}
            else if (firstNode.isLeaf() && secondNode.isLeaf()) {
				auto fnLvl = firstSg.level(firstNode);
				auto snLvl = secondSg.level(secondNode);
				sserialize::ItemIndex result;
				if (fnLvl == snLvl) {
					result = firstSg.items(firstNode) - secondSg.items(secondNode);
				}
				else if (fnLvl < snLvl) { //firstNode is an ancestor of secondNode
					SSERIALIZE_NORMAL_ASSERT(firstSg.sg().isAncestor(firstNode.pixelId(), secondNode.pixelId()));
					if (firstNode.isFullMatch()) {
						result = firstSg.sgi().items(secondNode.pixelId()) - secondSg.items(secondNode);
					}
					else {
						result = (firstSg.items(firstNode) / firstSg.sgi().items(secondNode.pixelId())) - secondSg.items(secondNode);
					}
				}
				else if (snLvl < fnLvl) { //secondNode is an ancestor of firstNode
					SSERIALIZE_NORMAL_ASSERT(firstSg.sg().isAncestor(secondNode.pixelId(), firstNode.pixelId()));
					SSERIALIZE_CHEAP_ASSERT(!secondNode.isFullMatch());
					result = firstSg.items(firstNode) - (secondSg.items(secondNode) / secondSg.sgi().items(firstNode.pixelId()));
				}
                if (!result.size()) {
                    rptr = std::unique_ptr<TreeNode>();
                }
                dest.m_fetchedItems.emplace_back(result);
                rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
            }
            else {
                rptr = TreeNode::make_unique(resultPixelId(firstNode, secondNode), TreeNode::IS_INTERNAL);

                if (firstNode.isInternal() && secondNode.isInternal()) {
                    auto fIt = firstNode.children().begin();
                    auto fEnd = firstNode.children().end();
                    auto sIt = secondNode.children().begin();
                    auto sEnd = secondNode.children().end();
                    for(;fIt != fEnd && sIt != sEnd;) {
                        if ((*fIt)->pixelId() < (*sIt)->pixelId()) {
							rptr->children().emplace_back( deepCopy(firstSg, **fIt) );
                            ++fIt;
                        }
                        else if ((*fIt)->pixelId() > (*sIt)->pixelId()) {
                            ++sIt;
                        }
                        else {
                            auto x = (*this)(**fIt, **sIt);
                            if (x) {
                                rptr->children().emplace_back(std::move(x));
                            }
                            ++fIt;
							++sIt;
                        }
                    }
                    for(; fIt != fEnd; ++fIt) {
						rptr->children().emplace_back( deepCopy(firstSg, **fIt) );
					}
                }
                else if (firstNode.isInternal()) {
                    auto fIt = firstNode.children().begin();
                    auto fEnd = firstNode.children().end();
                    for( ;fIt != fEnd; ++fIt) {
                        auto x = (*this)(**fIt, secondNode);
                        if (x) {
                            rptr->children().emplace_back(std::move(x));
                        }
                    }
                }
                else if (secondNode.isInternal()) {
					//This is more compicated:
					//The left side may have child nodes that are NOT part of the second operand
					//The items of these nodes do not get pruned, we therefore have to create these on-the-fly
					SSERIALIZE_NORMAL_ASSERT_SMALLER_OR_EQUAL(firstSg.level(firstNode), secondSg.level(secondNode));
					
					std::vector<PixelId> virtFirstPids = secondSg.pixelChildren(secondNode.pixelId());
                    auto sIt = secondNode.children().begin();
                    auto sEnd = secondNode.children().end();
					auto fIt = virtFirstPids.begin();
					auto fEnd = virtFirstPids.end();
					if (firstNode.isFullMatch()) {
						for(;fIt != fEnd && sIt != sEnd; ++fIt) {
							TreeNodePtr x;
							if (*fIt < (*sIt)->pixelId()) {
								rptr->children().emplace_back(TreeNode::make_unique(*fIt, TreeNode::IS_FULL_MATCH));
							}
							else {
								SSERIALIZE_ASSERT_EQUAL(*fIt, (*sIt)->pixelId());
								auto x = (*this)(firstNode, **sIt);
								if (x) {
									rptr->children().emplace_back(std::move(x));
								}
								++sIt;
							}
						}
						for(; fIt != fEnd; ++fIt) {
							rptr->children().emplace_back(TreeNode::make_unique(*fIt, TreeNode::IS_FULL_MATCH));
						}
					}
					else {
						sserialize::ItemIndex firstNodeItems = firstSg.items(firstNode);
						for(;fIt != fEnd && sIt != sEnd; ++fIt) {
							TreeNodePtr x;
							if (*fIt < (*sIt)->pixelId()) {
								auto result = firstNodeItems / firstSg.sgi().items(*fIt);
								if (result.size()) {
									dest.m_fetchedItems.emplace_back(result);
									x = TreeNode::make_unique(*fIt, TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
								}
							}
							else { //second < first should not happen since virtFirstPids contains ALL children
								SSERIALIZE_ASSERT_EQUAL(*fIt, (*sIt)->pixelId());
								x = (*this)(firstNode, **sIt);
								++sIt;
							}
							if (x) {
								rptr->children().emplace_back(std::move(x));
							}
						}
						for(; fIt != fEnd; ++fIt) {
							auto result = firstNodeItems / firstSg.sgi().items(*fIt);
							if (result.size()) {
								dest.m_fetchedItems.emplace_back(result);
								rptr->children().emplace_back( TreeNode::make_unique(*fIt, TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1) );
							}
						}
					}
                }

                if (!rptr->children().size()) {
                    rptr = TreeNodePtr();
                }
            }
			#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
				if (rptr) {
					if ((firstSg.items(firstNode) / firstSg.sgi().items(rptr->pixelId())) - (secondSg.items(secondNode) / secondSg.sgi().items(rptr->pixelId())) != dest.items(*rptr)) {
						auto sdiff = ((firstSg.items(firstNode) / firstSg.sgi().items(rptr->pixelId())) - (secondSg.items(secondNode) / secondSg.sgi().items(rptr->pixelId()))) ^ dest.items(*rptr);
						std::cout << "In result:\n";
						for (auto const & c : rptr->children()) {
							auto b = dest.items(*c) / sdiff;
							for(auto x : b) {
								std::cout << "item:" << x << " -> " << "pixel:" << c->pixelId() << '\n';
							}
						}
						std::cout << "In left:\n";
						if (firstNode.isInternal()) {
							for (auto const & c : firstNode.children()) {
								auto b = firstSg.items(*c) / sdiff;
								for(auto x : b) {
									std::cout << "item:" << x << " -> " << "pixel:" << c->pixelId() << '\n';
								}
							}
						}
						else {
							auto b = firstSg.items(firstNode) / sdiff;
							for(auto x : b) {
								std::cout << "item:" << x << " -> " << "pixel:" << firstNode.pixelId() << '\n';
							}
						}
						std::cout << "In right:\n";
						if (secondNode.isInternal()) {
							for (auto const & c : secondNode.children()) {
								auto b = secondSg.items(*c) / sdiff;
								for(auto x : b) {
									std::cout << "item:" << x << " -> " << "pixel:" << c->pixelId() << '\n';
								}
							}
						}
						else {
							auto b = secondSg.items(secondNode) / sdiff;
							for(auto x : b) {
								std::cout << "item:" << x << " -> " << "pixel:" << secondNode.pixelId() << '\n';
							}
							
						}
						sserialize::breakHereIf(sdiff.size());
					}
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL((firstSg.items(firstNode) / firstSg.sgi().items(rptr->pixelId())) - (secondSg.items(secondNode) / secondSg.sgi().items(rptr->pixelId())), dest.items(*rptr));
				}
				else {
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(0, ((firstSg.items(firstNode) / firstSg.sgi().items(rptr->pixelId())) - (secondSg.items(secondNode) / secondSg.sgi().items(rptr->pixelId()))).size());
				}
			#endif
			return rptr;
        }
    };
    sserialize::RCPtrWrapper<Self> dest( new Self(m_items, sgPtr(), sgiPtr()) );
	if (m_root && static_cast<Self const &>(other).m_root) {
		Recurser rec(*this, static_cast<Self const &>(other), *dest);
		dest->m_root = rec(*(this->m_root), *(static_cast<Self const &>(other).m_root));
	}
	SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(items() - other.items(), dest->items());
    return dest;
}

HCQRSpatialGrid::HCQRPtr
HCQRSpatialGrid::compactified(SizeType maxPMLevel) const {
    struct Recurser {
        HCQRSpatialGrid const & that;
        HCQRSpatialGrid & dest;
        SizeType maxPMLevel;
        Recurser(HCQRSpatialGrid const & that, HCQRSpatialGrid & dest, SizeType maxPMLevel) :
        that(that),
        dest(dest),
        maxPMLevel(maxPMLevel)
        {}
        std::unique_ptr<TreeNode> operator()(TreeNode const & node) const {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (node.isInternal()) {
                TreeNode::Children children;
				int flags = 0;
                for(auto const & x : node.children()) {
                    children.emplace_back((*this)(*x));
					flags |= children.back()->flags();
                }
                //check if we can compactify even further by merging partial-match indexes to parent nodes
                if ((flags & (TreeNode::IS_FULL_MATCH | TreeNode::IS_INTERNAL)) == 0 && that.sg().level(node.pixelId()) > sserialize::narrow_check<int>(maxPMLevel)) {
                    sserialize::SizeType dataSize = 0;
                    std::vector<sserialize::ItemIndex> indexes;
                    for(auto & x : children) {
                        indexes.emplace_back( dest.items(*x) );
                        dataSize += indexes.back().getSizeInBytes();
                    }
                    sserialize::ItemIndex merged = sserialize::ItemIndex::unite(indexes);
                    if (merged.getSizeInBytes() < dataSize) {
                        //we first have to get rid of those extra fetched indexes
                        for(auto rit(children.rbegin()), rend(children.rend()); rit != rend; ++rit) {
                            if ((*rit)->isFetched()) {
                                SSERIALIZE_CHEAP_ASSERT((*rit)->itemIndexId() == dest.m_fetchedItems.size()-1);
                                dest.m_fetchedItems.pop_back();
                            }
                        }
                        dest.m_fetchedItems.emplace_back(merged);
                        auto result = TreeNode::make_unique(node.pixelId(), TreeNode::IS_FETCHED, dest.m_fetchedItems.size()-1);
						SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(that.items(node), dest.items(*result));
						return result;
                    } 
                }
                //merging was not possible
                if ((flags & (~TreeNode::IS_FULL_MATCH)) || that.sg().childrenCount(node.pixelId()) != children.size()) {
                    auto result = node.shallowCopy(std::move(children));
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(that.items(node), dest.items(*result));
					return result;
                }
                else {
					auto result = TreeNode::make_unique(node.pixelId(), TreeNode::IS_FULL_MATCH);
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(that.items(node), dest.items(*result));
					return result;
                }
            }
            else if (node.isFetched()) {
                dest.m_fetchedItems.emplace_back( that.items(node) );
                auto result = node.shallowCopy(dest.m_fetchedItems.size()-1);
				SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(that.items(node), dest.items(*result));
				return result;
            }
            else {
                auto result = node.shallowCopy();
				SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(that.items(node), dest.items(*result));
				return result;
            }
        };
    };

    sserialize::RCPtrWrapper<Self> dest( new Self(m_items, sgPtr(), sgiPtr()) );
	if (m_root) {
		Recurser rec(*this, *dest, maxPMLevel);
		dest->m_root = rec(*m_root);
	}
    return dest;
}


HCQRSpatialGrid::HCQRPtr
HCQRSpatialGrid::expanded(SizeType level) const {
    struct Recurser: HCQRSpatialGridOpHelper {
        HCQRSpatialGrid const & that;
        SizeType level;
        Recurser(HCQRSpatialGrid & dest, HCQRSpatialGrid const & that, SizeType level) :
        HCQRSpatialGridOpHelper(dest),
        that(that),
        level(level)
        {}
        std::unique_ptr<TreeNode> operator()(TreeNode const & node) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            return rec(node, 0);
        }
        std::unique_ptr<TreeNode> rec(TreeNode const & node, SizeType myLevel) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (myLevel >= level) {
                return deepCopy(that, node);
            }
            if (node.isInternal()) {
                TreeNode::Children children;
                for(auto const & x : node.children()) {
                    children.emplace_back( rec(*x, myLevel+1) );
                }
                return node.shallowCopy(std::move(children));
            }
            else if (node.isFullMatch()) {
                auto result = node.shallowCopy();
                expandFullMatchNode(*result, myLevel);
                return result;
            }
            else {
                auto result = TreeNode::make_unique(node.pixelId(), TreeNode::IS_INTERNAL);
                expandPartialMatchNode(*result, myLevel, that.items(node));
                return result;
            }
        }
        void expandFullMatchNode(TreeNode & node, SizeType myLevel) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (myLevel >= level || !that.sg().childrenCount(node.pixelId())) {
                return;
            }
            node.setFlags(TreeNode::IS_INTERNAL);
            auto childrenCount = that.sg().childrenCount(node.pixelId());
            for(decltype(childrenCount) i(0); i < childrenCount; ++i) {
                node.children().emplace_back(TreeNode::make_unique(that.sg().index(node.pixelId(), i), TreeNode::IS_FULL_MATCH));
            }
            sortChildren(node);
            for(auto & x : node.children()) {
                expandFullMatchNode(*x, myLevel+1);
            }
        }
        void expandPartialMatchNode(TreeNode & node, SizeType myLevel, sserialize::ItemIndex const & items) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (myLevel >= level || !that.sg().childrenCount(node.pixelId())) {
                dest.m_fetchedItems.emplace_back(items);
                node.setItemIndexId(dest.m_fetchedItems.size()-1);
                node.setFlags(TreeNode::IS_FETCHED);
                return;
            }
            auto childrenCount = that.sg().childrenCount(node.pixelId());
            for(decltype(childrenCount) i(0); i < childrenCount; ++i) {
                auto childPixelId = that.sg().index(node.pixelId(), i);
                sserialize::ItemIndex childFmIdx = that.sgi().items(childPixelId);
                sserialize::ItemIndex childPmIdx = childFmIdx / items;
                node.children().emplace_back(TreeNode::make_unique(childPixelId, TreeNode::IS_INTERNAL));
                expandPartialMatchNode(*node.children().back(), myLevel+1, childPmIdx);
            }
        }
    };
    sserialize::RCPtrWrapper<Self> dest( new Self(m_items, sgPtr(), sgiPtr()) );
	if (m_root) {
		Recurser rec(*dest, *this, level);
		dest->m_root = rec(*m_root);
	}
    return dest;
}

HCQRSpatialGrid::HCQRPtr
HCQRSpatialGrid::allToFull() const {
    struct Recurser {
        std::unique_ptr<TreeNode> operator()(TreeNode const & node) {
			SSERIALIZE_NORMAL_ASSERT(node.valid());
            if (node.isInternal()) {
                TreeNode::Children children;
                for(auto const & x : node.children()) {
                    children.emplace_back((*this)(*x));
                }
                return node.shallowCopy(std::move(children));
            }
            else {
                return TreeNode::make_unique(node.pixelId(), TreeNode::IS_FULL_MATCH);
            }
        }
    };
    sserialize::RCPtrWrapper<Self> dest( new Self(m_items, sgPtr(), sgiPtr()) );
	if (m_root) {
		Recurser rec;
		dest->m_root = rec(*m_root);
	}
    return dest;
}

std::unique_ptr<HCQRSpatialGrid::TreeNode> const &
HCQRSpatialGrid::root() const {
	return m_root;
}

sserialize::ItemIndex
HCQRSpatialGrid::items(TreeNode const & node) const {
	SSERIALIZE_NORMAL_ASSERT(node.valid());
	if (node.isInternal()) {
		std::vector<ItemIndex> tmp;
		for(auto const & x : node.children()) {
			tmp.emplace_back(items(*x));
		}
		return ItemIndex::unite(tmp);
	}
	else if (node.isFullMatch()) {
		return sgi().items(node.pixelId());
	}
	else if (node.isFetched()) {
		return fetchedItems().at(node.itemIndexId());
	}
	else {
		return idxStore().at(node.itemIndexId());
	}
}

HCQRSpatialGrid::PixelLevel
HCQRSpatialGrid::level(TreeNode const & node) const {
	return Parent::level(node.pixelId());
}

void
HCQRSpatialGrid::flushFetchedItems(sserialize::ItemIndexFactory & idxFactory) {
	if (!fetchedItems().size()) {
		return;
	}
    struct Recurser {
		HCQRSpatialGrid & that;
		sserialize::ItemIndexFactory & idxFactory;
		Recurser(HCQRSpatialGrid & that, sserialize::ItemIndexFactory & idxFactory) :
		that(that), idxFactory(idxFactory)
		{}
        void operator()(TreeNode & node) const {
			if (node.isLeaf() && node.isFetched()) {
				node.setItemIndexId(
					idxFactory.addIndex(
						that.fetchedItems().at(
							node.itemIndexId()
						)
					)
				);
			}
			for(auto & x : node.children()) {
				(*this)(*x);
			}
        }
    };
	Recurser(*this, idxFactory)(*m_root);
	m_fetchedItems.clear();
}

} //end namespace sserialize::spatial::dgg::impl
