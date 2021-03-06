#ifndef SSERIALIZE_STATIC_TRIE_NODE_H
#define SSERIALIZE_STATIC_TRIE_NODE_H
#include <sserialize/Static/TrieNodePrivate.h>
#include <sserialize/utility/refcounting.h>

namespace sserialize {
namespace Static {


class TrieNode: public RCWrapper<TrieNodePrivate> {
public:
	enum Types {
		T_EMPTY, T_SIMPLE, T_COMPACT, T_LARGE_COMPACT
	};
private:
	uint32_t getSubTriePtrOffset() const;
	uint32_t getChildPtrBeginOffset() const;

public:

	TrieNode(TrieNodePrivate * node) : RCWrapper<TrieNodePrivate>(node) {}

	TrieNode(const TrieNode & other) : RCWrapper<TrieNodePrivate>(other) {}

	inline TrieNode & operator=(const TrieNode & other) {
		RCWrapper<TrieNodePrivate>::operator=(other);
		return *this;
	}

	virtual ~TrieNode() {}
	inline uint32_t childCount() const { return priv()->childCount();}
	inline uint8_t charWidth() const { return priv()->charWidth(); }
	inline uint32_t childCharAt(const uint32_t pos) const { return priv()->childCharAt(pos);}
	inline uint8_t strLen() const { return priv()->strLen(); }
	inline UByteArrayAdapter strData() const { return priv()->strData(); }
	inline std::string str() const { return priv()->str(); }
	inline int32_t posOfChar(const uint32_t ucode) const { return priv()->posOfChar(ucode);};
	inline TrieNode childAt(const uint32_t pos) const { return TrieNode( priv()->childAt(pos) );}

	inline bool hasMergeIndex() const { return priv()->hasMergeIndex();}
	inline bool hasExactIndex() const { return priv()->hasExactIndex();}
	inline bool hasPrefixIndex() const { return priv()->hasPrefixIndex();}
	inline bool hasSuffixIndex() const { return priv()->hasSuffixIndex();}
	inline bool hasSuffixPrefixIndex() const { return priv()->hasSuffixPrefixIndex();}
	inline bool hasAnyIndex() const { return priv()->hasAnyIndex();}

	inline void dump() const { priv()->dump();}

	inline UByteArrayAdapter::SizeType getStorageSize() const { return priv()->getStorageSize(); }
	inline UByteArrayAdapter::SizeType getHeaderStorageSize() const { return priv()->getHeaderStorageSize(); }
	inline UByteArrayAdapter::SizeType getNodeStringStorageSize() const { return priv()->getNodeStringStorageSize();}
	inline UByteArrayAdapter::SizeType getChildPtrStorageSize() const { return priv()->getChildPtrStorageSize();}
	inline UByteArrayAdapter::SizeType getChildCharStorageSize() const { return priv()->getChildCharStorageSize();}
	inline UByteArrayAdapter::SizeType getIndexPtrStorageSize() const { return priv()->getIndexPtrStorageSize();}

	inline uint32_t getIndexPtr() const { return 0xFFFFFFFF;}
	inline uint32_t getExactIndexPtr() const { return priv()->getExactIndexPtr();}
	inline uint32_t getPrefixIndexPtr() const { return priv()->getPrefixIndexPtr();}
	inline uint32_t getSuffixIndexPtr() const { return priv()->getSuffixIndexPtr();}
	inline uint32_t getSuffixPrefixIndexPtr() const { return priv()->getSuffixPrefixIndexPtr();}

	inline uint32_t getChildPtr(const uint32_t pos) const { return priv()->getChildPtr(pos); }
};

}} //end namespace

#endif