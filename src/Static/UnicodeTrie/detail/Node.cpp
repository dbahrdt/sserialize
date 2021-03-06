#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/Static/UnicodeTrie/detail/SimpleNode.h>
#include <sserialize/Static/UnicodeTrie/detail/EmptyNode.h>
#include <sserialize/strings/stringfunctions.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

namespace detail {

void Node::dump(std::ostream& out) const {
	out << "sserialize::Static::UnicodeTrie::Node[size=" << childSize() << "; payloadPtr=" << payloadPtr();
	out << ";";
	for(uint32_t i(0), s(childSize()); i < s; ++i) {
		uint32_t cp = childKey(i);
		out << "(" << sserialize::stringFromUnicodePoints(&cp, (&cp)+1) << "=>" << childPtr(i) << ")";
	}
	out << "]";
}

}

Node::Node() :
m_priv( new detail::EmptyNode() )
{}

Node RootNodeAllocator::operator()(uint32_t nodeType, const sserialize::UByteArrayAdapter & src) const {
	switch (nodeType) {
	case Node::NT_SIMPLE:
		return Node( new detail::SimpleNode(src) );
	default:
		return Node();
	};
}

}}}