#ifndef SSERIALIZE_GENERALIZED_TRIE_SINGLE_PASS_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_SINGLE_PASS_TRIE_H
#include <sserialize/storage/UByteArrayAdapter.h>
#if defined(SSERIALIZE_UBA_NON_CONTIGUOUS) || defined(SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL)
#include <sserialize/containers/GeneralizedTrie/SerializableTrie.h>
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/utility/log.h>

namespace sserialize {
namespace GeneralizedTrie {

class SinglePassTrie: public SerializableTrie< WindowedArray<uint32_t> > {
public:
	typedef SerializableTrie< WindowedArray<uint32_t> > MyBaseClass;
	typedef MyBaseClass::ItemSetContainer ItemSetContainer;
protected:
	class ParentTPNS: public Node::TemporalPrivateStorage {
	public:
		ParentTPNS() : TemporalPrivateStorage(), childrenCompleted(0) {}
		virtual ~ParentTPNS() {}
		uint32_t childrenCompleted;
		std::deque<uint32_t> staticChildPointers;
	};

	class ChildTPNS: public Node::TemporalPrivateStorage {
	public:
		ChildTPNS(OffsetType prefixIndexPtr, OffsetType subStrIndexPtr) : TemporalPrivateStorage(), prefixIndexPtr(prefixIndexPtr), subStrIndexPtr(subStrIndexPtr) {}
		virtual ~ChildTPNS() {}
		OffsetType prefixIndexPtr;
		OffsetType subStrIndexPtr;
	};
protected:
	sserialize::MmappedMemory<uint32_t> exactIndicesMem;
	sserialize::MmappedMemory<uint32_t> suffixIndicesMem;
	bool m_hasSuffixes;

private:

	bool handleNodeIndices(Node * node, sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig & config, sserialize::Static::TrieNodeCreationInfo & nodeInfo);

	template<class StaticTrieNodeT>
	void serializeTrieBottomUp(GeneralizedTrieCreatorConfig & config);
private:
	SinglePassTrie(const SinglePassTrie & other);
	SinglePassTrie & operator=(const SinglePassTrie & other);
public:
	SinglePassTrie();
	SinglePassTrie(bool caseSensitive);
	virtual ~SinglePassTrie();
	void swap(MyBaseClass::MyBaseClass & other);
	
	template<typename T_ITEM_FACTORY>
	bool fromStringsFactory(const T_ITEM_FACTORY& stringsFactory, const uint32_t begin, const uint32_t end, sserialize::MmappedMemoryType mt);
	
	void createStaticTrie(GeneralizedTrieCreatorConfig & config);
	
	using MyBaseClass::trieSerializationProblemFixer;
};


template<typename T_ITEM_FACTORY>
bool SinglePassTrie::fromStringsFactory(const T_ITEM_FACTORY & stringsFactory, const uint32_t begin, const uint32_t end, MmappedMemoryType mt) {
	StringsContainer itemPrefixStrings, itemSuffixStrings;
	std::unordered_map<std::string, std::unordered_set<Node*> > strIdToSubStrNodes;
	std::unordered_map<std::string, std::unordered_set<Node*> > strIdToExactNodes;
	uint64_t exactStorageNeed = 0;
	uint64_t suffixStorageNeed = 0;
	
	if (!MyBaseClass::MyBaseClass::trieFromStringsFactory<T_ITEM_FACTORY>(stringsFactory, begin, end, strIdToSubStrNodes, strIdToExactNodes)) {
		std::cout << "Failed to create trie" << std::endl;
		return false;
	}
	
	ProgressInfo progressInfo;
	progressInfo.begin(end-begin, "BaseTrie::fromStringsFactory::calculating storage need");
	std::unordered_set<Node*> exactNodes;
	std::unordered_set<Node*> suffixNodes;
	for(uint32_t i(0); i < end; ++i) {
		exactNodes.clear();
		suffixNodes.clear();
		itemPrefixStrings.clear();
		itemSuffixStrings.clear();
		stringsFactory(i, itemPrefixStrings, itemSuffixStrings);
		for(const std::string & str : itemPrefixStrings) {
			const std::unordered_set<Node*> & v = strIdToExactNodes.at(str);
			exactNodes.insert(v.begin(), v.end());
		}
		
		for(const std::string & str : itemSuffixStrings) {
			const std::unordered_set<Node*> & v = strIdToSubStrNodes.at(str);
			suffixNodes.insert(v.begin(), v.end());
		}
		
		exactStorageNeed += exactNodes.size();
		suffixStorageNeed += suffixNodes.size();

		for(std::unordered_set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.reserve((*esit)->exactValues.capacity()+1);
		}
		for(std::unordered_set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.reserve((*it)->subStrValues.capacity()+1);
		}
		if (suffixNodes.size()) {
			m_hasSuffixes = true;
		}
		progressInfo(i);
	}
	progressInfo.end();
	std::cout << std::endl;
	
	std::cout << "Creating temporary on-disk storage for indices" << std::endl;
	std::cout << "Exact index: " << sserialize::prettyFormatSize(exactStorageNeed*sizeof(uint32_t)) << std::endl;
	std::cout << "Suffix index: " << sserialize::prettyFormatSize(suffixStorageNeed*sizeof(uint32_t)) << std::endl;
	exactIndicesMem = sserialize::MmappedMemory<uint32_t>(exactStorageNeed, mt);
	suffixIndicesMem= sserialize::MmappedMemory<uint32_t>(suffixStorageNeed, mt);
	{
		uint32_t * exactIndicesPtr = exactIndicesMem.begin();
		uint32_t * suffixIndicesPtr = suffixIndicesMem.begin();
	
		auto indexStorageDistributor = [&exactIndicesPtr, &suffixIndicesPtr] (Node * node) {
			std::size_t cap = node->exactValues.capacity();
			node->exactValues = WindowedArray<uint32_t>(exactIndicesPtr, exactIndicesPtr+cap);
			exactIndicesPtr = exactIndicesPtr+cap;
			
			cap = node->subStrValues.capacity();
			node->subStrValues = WindowedArray<uint32_t>(suffixIndicesPtr, suffixIndicesPtr+cap);
			suffixIndicesPtr = suffixIndicesPtr+cap;
		};
		m_root->mapDepthFirst(indexStorageDistributor);
		SSERIALIZE_CHEAP_ASSERT(exactIndicesPtr == exactIndicesMem.begin()+exactStorageNeed);
		SSERIALIZE_CHEAP_ASSERT(suffixIndicesPtr == suffixIndicesMem.begin()+suffixStorageNeed);
	}
	std::cout << "Distributed memory to nodes" << std::endl;
	
	
	progressInfo.begin(end-begin, "SinglePassTrie::fromStringsFactory::popluating exact/suffix index");
	for(uint32_t i = 0; i < end; ++i) {
		exactNodes.clear();
		suffixNodes.clear();
		itemPrefixStrings.clear();
		itemSuffixStrings.clear();
		stringsFactory(i, itemPrefixStrings, itemSuffixStrings);
		for(const std::string & str : itemPrefixStrings) {
			const std::unordered_set<Node*> & v = strIdToExactNodes.at(str);
			exactNodes.insert(v.begin(), v.end());
		}
		
		for(const std::string & str : itemSuffixStrings) {
			const std::unordered_set<Node*> & v = strIdToSubStrNodes.at(str);
			suffixNodes.insert(v.begin(), v.end());
		}
		
		for(std::unordered_set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.push_back(i);
		}
		for(std::unordered_set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.push_back(i);
		}
		
		progressInfo(i);
	}
	progressInfo.end();
	
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after SinglePassTrie::fromStringsFactory::insertItems" << std::endl;
		return false;
	}

	return true;
}

/**
 * This function serializes the trie in bottom-up fashion
 *
 */

template<class StaticTrieNodeT>
void
SinglePassTrie::
serializeTrieBottomUp(GeneralizedTrieCreatorConfig & config) {
	uint32_t depth = getDepth();
	if (depth == 0) {
		std::cout << "Trie is empty" << std::endl;
		return;
	}

	if (!consistencyCheck()) {
		std::cout << "Trie is broken!" << std::endl;
		return;
	}

	ProgressInfo progressInfo;
	progressInfo.begin(m_nodeCount, "SinglePassTrie::Bottom-up-Serialization");
	uint32_t finishedNodeCount = 0;
	std::cout << "Serializing Trie:" << std::endl;
	
	std::deque< std::deque< Node * >  > trieNodeQue;
	if (m_nodeCount < 200 * 1000 * 1000) {
		rootNode()->addNodesSorted(0, trieNodeQue);
	}
	else {
		trieNodeQue.resize(depth);
	}
	for(int curLevel((int)depth-1); curLevel >= 0; curLevel--) {
		std::deque<Node *> & nodeQue = trieNodeQue.at((uint32_t)curLevel);

		if (nodeQue.size() == 0) {
			rootNode()->addNodesSortedInLevel((uint16_t)curLevel, 0, nodeQue);
		}
		uint32_t curNodeCount = 0;
		for(std::deque< Node * >::const_reverse_iterator it = nodeQue.rbegin(); it != nodeQue.rend(); ++it) {
			Node * curNode = *it;
			
			Static::TrieNodeCreationInfo nodeInfo;

			if (curNode->c.size()) {
				std::string::const_iterator nodeStrIt(curNode->c.cbegin());
				utf8::next(nodeStrIt, curNode->c.cend());
				nodeInfo.nodeStr = std::string(nodeStrIt, curNode->c.cend());
			}
			curNode->insertSortedChildChars(nodeInfo.childChars);

			if (nodeInfo.childChars.size() != curNode->children.size()) {
				std::cout << "Error:  nodeInfo.childChars.size()=" << nodeInfo.childChars.size() << "!= curNode->children.size()=" << curNode->children.size() << std::endl;
			}
			
			//Get our child ptrs
			if (curNode->temporalPrivateStorage) {
				std::deque<uint32_t> tmp;
				ParentTPNS * tmpStorage = dynamic_cast<ParentTPNS*>(curNode->temporalPrivateStorage);
				tmpStorage->staticChildPointers.swap(tmp);

				//now do the remapping for the relative ptrs. stored ptrs offsets from the end of the list
				for(size_t i = 0; i < tmp.size(); i++) {
					nodeInfo.childPtrs.push_back((uint32_t)(config.trieList->size() - tmp[i]));
				}
			}
			
			if (nodeInfo.childChars.size() != nodeInfo.childPtrs.size()) {
				std::cout << "Error: (childChars.size() != childPtrs.size()) = (" << nodeInfo.childChars.size() << " != " << nodeInfo.childPtrs.size() << ")" << std::endl;
				curNode->dump();
			}

			if (!handleNodeIndices(curNode, config, nodeInfo)) {
				std::cout <<  "Failed to handle node Item index" << std::endl;
			}

			unsigned int err =  StaticTrieNodeT::prependNewNode(nodeInfo, *(config.trieList));
			if (StaticTrieNodeT::isError(err)) {
				std::cout << "ERROR: " << StaticTrieNodeT::errorString(err) << std::endl;
			}

			//Add ourself to the child ptrs of our parent
			if (curNode->parent) {
				ParentTPNS * parentTempStorage = dynamic_cast<ParentTPNS*>(curNode->parent->temporalPrivateStorage);
				if (!parentTempStorage) {
					parentTempStorage = new ParentTPNS();
					curNode->parent->temporalPrivateStorage = parentTempStorage;
				}
				uint32_t nodeOffset = (uint32_t) config.trieList->size();
				parentTempStorage->staticChildPointers.push_front(nodeOffset);
			}

			
			//delete children if config says so
			if (config.deleteRootTrie) {
				curNode->deleteChildren();
			}
			else {//Delete all child temporal node storages as those are not needed anymore
				for(Node::ChildNodeIterator it = curNode->children.begin(); it != curNode->children.end(); ++it) {
					it->second->deleteTemporalPrivateStorage();
				}
			}

			++curNodeCount;
			++finishedNodeCount;
			progressInfo(finishedNodeCount);
		}
		trieNodeQue.pop_back();
	}
	progressInfo.end();
	rootNode()->deleteTemporalPrivateStorage();
}

}}//end namespace

#endif
#endif