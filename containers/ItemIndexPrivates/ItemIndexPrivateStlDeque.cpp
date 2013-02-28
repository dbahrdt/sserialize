#include "ItemIndexPrivateStlDeque.h"
#include <algorithm>

namespace sserialize {

ItemIndexPrivateStlDeque::ItemIndexPrivateStlDeque() : ItemIndexPrivate() {}
ItemIndexPrivateStlDeque::ItemIndexPrivateStlDeque(const std::deque<uint32_t> & data) : ItemIndexPrivate(), m_data(data) {}
ItemIndexPrivateStlDeque::~ItemIndexPrivateStlDeque() {}

ItemIndex::Types ItemIndexPrivateStlDeque::type() const {
	return ItemIndex::T_STL_DEQUE;
}

uint32_t ItemIndexPrivateStlDeque::at(uint32_t pos) const {
    return m_data.at( std::min(static_cast<uint32_t>(m_data.size()-1), pos) );
}

uint32_t ItemIndexPrivateStlDeque::first() const {
	return m_data.front();
}

uint32_t ItemIndexPrivateStlDeque::last() const {
	return m_data.back();
}

uint32_t ItemIndexPrivateStlDeque::size() const {
	return m_data.size();
}

}//end namespace