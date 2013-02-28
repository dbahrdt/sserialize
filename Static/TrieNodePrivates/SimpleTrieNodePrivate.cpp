#include "SimpleTrieNodePrivate.h"
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/find_key_in_array_functions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/vendor/libs/utf8/source/utf8.h>
#include <sserialize/utility/utilfuncs.h>
#include <iostream>
#include "../TrieNode.h"

#define CHILDPTR_STRIPE_SIZE 32

namespace sserialize {
namespace Static {


SimpleTrieNodePrivate::SimpleTrieNodePrivate(const UByteArrayAdapter & nodeData)  : m_data(nodeData) {
	uint32_t curOffSet = 0;
	
	m_childCount = m_data.getUint16(curOffSet);
	curOffSet += 2;

	m_charWidth = m_data.getUint8(curOffSet);
	curOffSet += 1;

	m_indexTypes = (IndexTypes) m_data.getUint8(curOffSet);
	curOffSet += 1;

	m_strLen = m_data.getUint8(curOffSet);
	curOffSet += 1;

	m_strStart = curOffSet;
	curOffSet += m_strLen;

	m_childArrayStart = curOffSet;
	curOffSet += m_childCount*4; //charwidth is  always 4

	m_childPointerArrayStart = curOffSet;
	curOffSet += m_childCount*4;

	m_indexPtrStart = curOffSet;
	curOffSet += 16;
	
	m_myEndPtr = curOffSet;
}

uint32_t SimpleTrieNodePrivate::getChildPtrBeginOffset() const {
	return m_childPointerArrayStart;
}

uint32_t SimpleTrieNodePrivate::childCharAt(uint16_t pos) const {
	if (m_childCount == 0)
		return 0;
	if (pos >= m_childCount) {
		pos = m_childCount-1;
	}
	return m_data.getUint32(m_childArrayStart+4*pos);
}

uint32_t SimpleTrieNodePrivate::getExactIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart);
	return r;
}

uint32_t SimpleTrieNodePrivate::getPrefixIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart+4);
	return r;
}


uint32_t SimpleTrieNodePrivate::getSuffixIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart+8);
	return r;
}

uint32_t SimpleTrieNodePrivate::getSuffixPrefixIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart+12);
	return r;
}

uint32_t SimpleTrieNodePrivate::getChildPtr(uint32_t pos) const {
	return m_data.getUint32(m_childPointerArrayStart+4*pos);
}

UByteArrayAdapter SimpleTrieNodePrivate::strData() const {
	return UByteArrayAdapter(m_data, m_strStart, m_strLen);
}


std::string SimpleTrieNodePrivate::str() const {
	std::string st;
	for(int i = 0; i < m_strLen; i++)
		st.append(1, static_cast<char>(m_data.at(m_strStart+i)));
	return st;
}

int16_t SimpleTrieNodePrivate::posOfChar(uint32_t key) const {
	if (key > 0xFFFF || (m_charWidth == 1 && key > 0xFF))
		return -1;
	return findKeyInArray_uint32<4>(m_data+m_childArrayStart, m_childCount, key);
}

TrieNodePrivate* SimpleTrieNodePrivate::childAt(uint16_t pos) const {
	if (pos >= m_childCount)
		pos = (m_childCount-1);
	uint32_t childOffSet = getChildPtr(pos);
	return new SimpleTrieNodePrivate(m_data + (m_myEndPtr+childOffSet));
}

void SimpleTrieNodePrivate::dump() const {
	std::cout << "[";
	std::cout << "m_current[0]=" << static_cast<int>(m_data.at(0)) << ";";
	std::cout << "m_charWidth=" << static_cast<int>(m_charWidth) << "; ";
	std::cout << "m_childCount=" << static_cast<int>(m_childCount) << "; ";
	std::cout << "m_strLen=" << static_cast<int>(m_strLen) << "; ";
	std::cout << "str=" << str() << ";";
	std::cout << "mergeIndex=" << (hasMergeIndex() ? "true" : "false") << "; ";
	std::cout << "exactIndexPtr=" << getExactIndexPtr() << ";";
	std::cout << "prefixIndexPtr=" << getPrefixIndexPtr() << ";";
	std::cout << "suffixIndexPtr=" << getSuffixIndexPtr() << ";";
	std::cout << "suffixPrefixIndexPtr=" << getSuffixPrefixIndexPtr() << ";";
	std::cout << "childrenChars=(";
	std::string childCharStr;
	for(uint16_t i = 0; i < m_childCount; i++) {
		utf8::append(childCharAt(i), std::back_inserter(childCharStr));
		childCharStr += ", ";
	}
	std::cout << childCharStr << ")]" << std::endl << std::flush;
}

uint32_t SimpleTrieNodePrivate::getStorageSize() const {
	return 5+strLen()+8*m_childCount+16;
}

uint32_t SimpleTrieNodePrivate::getHeaderStorageSize() const {
	return 4;
}


uint32_t SimpleTrieNodePrivate::getNodeStringStorageSize() const {
	return 1 + strLen();
}

uint32_t SimpleTrieNodePrivate::getChildPtrStorageSize() const {
	return m_childCount*4;
}

uint32_t SimpleTrieNodePrivate::getChildCharStorageSize() const {
	return 4*m_childCount;
}

uint32_t SimpleTrieNodePrivate::getIndexPtrStorageSize() const {
	return 16;
}


SimpleStaticTrieCreationNode::SimpleStaticTrieCreationNode(const UByteArrayAdapter & nodeData) : m_node(nodeData), m_data(nodeData) {};


bool SimpleStaticTrieCreationNode::setChildPointer(uint32_t childNum, uint32_t offSetFromBeginning) {
	if (m_node.childCount() <= childNum)
		return false;
	m_data.putUint32(m_node.getChildPtrBeginOffset()+4*childNum, offSetFromBeginning);
	return true;
}

unsigned int
SimpleStaticTrieCreationNode::createNewNode(
    const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination) {

	uint32_t childrenCount = nodeInfo.childChars.size();

	if (childrenCount > 0xFFFF) {
		std::cout << "Error: too many children" << std::endl;
		return SimpleStaticTrieCreationNode::TOO_MANY_CHILDREN;
	}
	

	destination.putUint16(childrenCount);

	destination.putUint8(nodeInfo.charWidth);

	destination.putUint8(nodeInfo.indexTypes | (nodeInfo.mergeIndex ? Static::TrieNodePrivate::IT_MERGE_INDEX : 0));
	
	//Possibly push our node string
	uint32_t nodeStrSize = 0;
	if (nodeInfo.nodeStr.size() > 0) {
		//push the string with string length
		std::string::const_iterator nodeStrIt = nodeInfo.nodeStr.begin();
		utf8::next(nodeStrIt, nodeInfo.nodeStr.end()); //remove the first char as that one is already in our parent
		std::string stnodeStr;
		for(; nodeStrIt != nodeInfo.nodeStr.end(); nodeStrIt++)
			stnodeStr += *nodeStrIt;
		
		nodeStrSize = stnodeStr.size();
		if (nodeStrSize <= 0xFF) {
			destination.putUint8(nodeStrSize);
		}
		else {
			std::cout << "node string is too long: " << nodeInfo.nodeStr << std::endl;
			return SimpleStaticTrieCreationNode::NODE_STRING_TOO_LONG;
		}
		//Node string (not null terminated)

		for(std::string::iterator i = stnodeStr.begin(); i != stnodeStr.end(); i++) {
			destination.putUint8(static_cast<uint8_t>(*i));
		}
	}
	else { // no nodestring there
		destination.putUint8(0);
	}

	//Push the child chars and child ptrs
	if (childrenCount) {
		for(unsigned int i=0; i < childrenCount; i++) {
			destination.putUint32(nodeInfo.childChars[i]);
		}

		//dummy pointer values
		for(unsigned int j = 0; j < childrenCount; j++) {
			destination.putUint32(nodeInfo.childPtrs[j]);
		}
	}
	
	//Push index pointers
	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_EXACT) {
		destination.putUint32(nodeInfo.exactIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_PREFIX) {
		destination.putUint32(nodeInfo.prefixIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_SUFFIX) {
		destination.putUint32(nodeInfo.suffixIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_SUFFIX_PREFIX) {
		destination.putUint32(nodeInfo.suffixPrefixIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	return SimpleStaticTrieCreationNode::NO_ERROR;
}


unsigned int SimpleStaticTrieCreationNode::appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination) {
	return SimpleStaticTrieCreationNode::createNewNode(nodeInfo, destination);
}

unsigned int
SimpleStaticTrieCreationNode::
prependNewNode(const TrieNodeCreationInfo & nodeInfo, std::deque< uint8_t >& destination)
{
	std::vector< uint8_t > tmpDest;
	UByteArrayAdapter tmpDestAdap(&tmpDest);
	unsigned int err = SimpleStaticTrieCreationNode::appendNewNode(nodeInfo, tmpDestAdap);
	prependToDeque(tmpDest, destination);
	return err;
}

bool SimpleStaticTrieCreationNode::isError(unsigned int error) {
	return (error !=  SimpleStaticTrieCreationNode::NO_ERROR);
}

std::string SimpleStaticTrieCreationNode::errorString(unsigned int error) {
	std::string estr = "";
	if (error & SimpleStaticTrieCreationNode::TOO_MANY_CHILDREN) estr += "Too many children! ";
	if (error & SimpleStaticTrieCreationNode::CHILD_PTR_FAILED) estr += "Failed to set child ptr! ";
	if (error & SimpleStaticTrieCreationNode::NODE_STRING_TOO_LONG) estr += "Nodestring too long! ";
	return estr;
}


uint8_t SimpleStaticTrieCreationNode::getType() {
	return TrieNode::T_SIMPLE;
}

}}//end namespace