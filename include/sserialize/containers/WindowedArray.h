#ifndef SSERIALIZE_WINDOWED_ARRAY
#define SSERIALIZE_WINDOWED_ARRAY
#include <sserialize/utility/exceptions.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/assert.h>
#include <algorithm>
#include <iterator>

namespace sserialize {

template<typename T_VALUE>
class WindowedArray final {
public:
	typedef T_VALUE * iterator;
	typedef const T_VALUE * const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef std::size_t SizeType;
private:
	T_VALUE * m_begin;
	T_VALUE * m_end;
	T_VALUE * m_push;
public:
	WindowedArray() : m_begin(0), m_end(0), m_push(0) {}
	WindowedArray(T_VALUE * begin, T_VALUE * end) : m_begin(begin), m_end(end), m_push(begin) {}
	WindowedArray(T_VALUE * begin, T_VALUE * end, T_VALUE * push) : m_begin(begin), m_end(end), m_push(push) {}
	WindowedArray(const WindowedArray & other) : m_begin(other.m_begin), m_end(other.m_end), m_push(other.m_push) {}
	WindowedArray & operator=(const WindowedArray & other) {
		m_begin = other.m_begin;
		m_end = other.m_end;
		m_push = other.m_push;
		return *this;
	}
	
	SizeType size() const { return (SizeType)(m_push - m_begin); }
	SizeType capacity() const { return m_end - m_begin; }
	///This does not check if the new size is valid!
	void reserve(SizeType newCap) {
		m_end = m_begin+newCap;
	}
	
	///This does NOT free the associated memory 
	~WindowedArray() {}
	
	///This does NOT free the associated memory 
	void clear() {
		m_push = m_begin;
	}
	
	void push_back(const T_VALUE & value) {
		if (m_push < m_end) {
			*m_push = value;
			++m_push;
		}
		else {
			throw sserialize::OutOfBoundsException("WindowedArray::push_back");
		}
	}
	
	///This does not check if it is in the range
	iterator insert(iterator it, const T_VALUE & value) {
		*it = value;
		return ++it;
	}
	
	T_VALUE & operator[](SizeType pos) { return *(m_begin+pos); }
	const T_VALUE & operator[](SizeType pos) const { return *(m_begin+pos); }
	
	void pop_back() {
		if (m_push > m_begin)
			--m_push;
	}
	
	T_VALUE & back() {
		if (m_push > m_begin)
			return *(m_push-1);
		throw sserialize::OutOfBoundsException("WindowedArray::back"); 
	}
	const T_VALUE & back() const {
		if (m_push > m_begin)
			return *(m_push-1);
		throw sserialize::OutOfBoundsException("WindowedArray::back");
	}
	
	iterator begin() { return m_begin; }
	iterator end() { return m_push; }
	iterator capacityEnd() { return m_end; }
	
	const_iterator begin() const { return m_begin; }
	const_iterator end() const { return m_push; }
	const_iterator capacityEnd() const { return m_end; }

	const_iterator cbegin() const { return m_begin; }
	const_iterator cend() const { return m_push; }
	
	reverse_iterator rbegin() { return reverse_iterator(m_end-1); }
	reverse_iterator rend() { return reverse_iterator(m_begin-1); }
	
	const_reverse_iterator rbegin() const { return reverse_iterator(m_end-1); }
	const_reverse_iterator rend() const { return reverse_iterator(m_begin-1); }
	
	const_reverse_iterator crbegin() const { return reverse_iterator(m_end-1); }
	const_reverse_iterator crend() const { return reverse_iterator(m_begin-1); }
	
	template<typename T_INPUT_ITERATOR>
	void push_back(T_INPUT_ITERATOR a, T_INPUT_ITERATOR b) {
		m_push = std::copy(a, b, m_push);
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL( m_push, m_end );
	}
	
	WindowedArray<T_VALUE> slice(SizeType begin, SizeType end) {
		return WindowedArray(m_begin+begin, m_begin+end);
	}
	
	///Unite operator unite the contents of the two Windowed Arrays and puts the contents into dest.
	///CAVEAT: a and b span dest!!!!
	template<typename T_UNITE_OPERATOR>
	static WindowedArray uniteSortedInPlace(WindowedArray a, WindowedArray b, T_UNITE_OPERATOR uniteOp) {
		if (b.m_end == a.m_begin) {
			if (a.m_end == b.m_begin) { //this means their size is zero and they begin at the same boundaries 
				return a;
			}
			return uniteSortedInPlace(b, a, uniteOp);
		}
		else {
			if (a.m_end != b.m_begin)
				throw sserialize::OutOfBoundsException("WindowedArray::uniteSortedInPlace: bounds don't match");
			WindowedArray dest(a.m_begin, b.m_end, a.m_begin);
			uniteOp(a, b, dest);
			return dest;
		}
	}
	
	static WindowedArray unite(WindowedArray a, WindowedArray b) {
		if (b.m_end == a.m_begin) {
			if (a.m_end == b.m_begin) { //this means their size is zero and they begin at the same boundaries 
				return a;
			}
			return unite(b, a);
		}
		else {
			if (a.m_end != b.m_begin)
				throw sserialize::OutOfBoundsException("WindowedArray::unite: bounds don't match");
			return WindowedArray(a.m_begin, b.m_end);
		}
	}
	
	static WindowedArray uniteSortedInPlace(WindowedArray a, WindowedArray b) {
		if (b.m_end == a.m_begin) {
			if (a.m_end == b.m_begin) { //this means their size is zero and they begin at the same boundaries 
				return a;
			}
			return uniteSortedInPlace(b, a);
		}
		else {
			if (a.m_end != b.m_begin)
				throw sserialize::OutOfBoundsException("WindowedArray::uniteSortedInPlace: bounds don't match");
			T_VALUE * tmp = new T_VALUE[a.size()+b.size()];
			T_VALUE * tmpBegin = tmp;
			T_VALUE * tmpEnd = std::set_union(a.m_begin, a.m_push, b.m_begin, b.m_push, tmpBegin);
			T_VALUE * d = std::copy(tmpBegin, tmpEnd, a.m_begin);
			delete[] tmp;
			return WindowedArray(a.m_begin, b.m_end, d);
		}
	}
	
	template<typename T_ITERATOR_TO_WINDOWED_ARRAYS>
	static WindowedArray uniteSortedInPlace(T_ITERATOR_TO_WINDOWED_ARRAYS begin, T_ITERATOR_TO_WINDOWED_ARRAYS end) {
		auto mergeFunc = [](WindowedArray a, WindowedArray b) { return WindowedArray::uniteSortedInPlace(a, b); };
		return sserialize::treeReduce(begin, end, mergeFunc);
	}
	
};

}//end namespace

#endif