#ifndef SSERIALIZE_STATIC_STRING_TABLE_H
#define SSERIALIZE_STATIC_STRING_TABLE_H
#include <unordered_set>
#include <sserialize/Static/Array.h>
#include <sserialize/search/StringCompleter.h>

namespace sserialize {
namespace Static {

/** Abstracts access to a string table,
 * strings should be in the order of most usage,
 * so that the smallest ids are most frequently used
 */
class StringTable: public RCWrapper< sserialize::Static::Array<std::string> > {
public:
	typedef sserialize::Static::Array<std::string>::const_iterator const_iterator;
	typedef sserialize::Static::Array<std::string>::const_reference const_reference;
	typedef sserialize::Static::Array<std::string>::SizeType SizeType;
	static constexpr SizeType npos = sserialize::Static::Array<std::string>::npos;
protected:
	inline UByteArrayAdapter & data() { return priv()->data();}
	inline const UByteArrayAdapter & data() const { return priv()->data(); }
public:
    StringTable();
    StringTable(const UByteArrayAdapter& data);
    StringTable(UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag);
    StringTable(UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag);
    StringTable(Static::Array< std::string > * data);
    StringTable(const StringTable & other);
	virtual ~StringTable();
	StringTable & operator=(const StringTable & other);
	/** @param qtype type of the query as */
	std::unordered_set< uint32_t > find(const std::string& searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	void find(std::unordered_set< uint32_t >& ret, std::string searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	bool match(uint32_t stringId, const std::string & searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	std::ostream& printStats(std::ostream & out) const;
	
	inline SizeType size() const { return priv()->size();}
	inline UByteArrayAdapter::OffsetType getSizeInBytes() const { return priv()->getSizeInBytes();}
	inline std::string at(SizeType pos) const { return priv()->at(pos); }
	inline UByteArrayAdapter strDataAt(SizeType pos) const { return priv()->dataAt(pos).getStringData(0);}
	inline uint32_t strSize(SizeType pos) const { return priv()->dataAt(pos).getStringLength(0); }
	inline std::string front() const { return priv()->front();}
	inline std::string back() const { return priv()->back();}
	inline const_iterator cbegin() const { return priv()->cbegin();}
	inline const_iterator begin() const { return priv()->cbegin();}
	inline const_iterator end() const { return priv()->cend();}
	inline const_iterator cend() const { return priv()->cend();}

	///The order of the strings impacts the type	
	template<typename T_RANDOM_ACCESS_ITERATOR, typename T_VALUE_TYPE = typename T_RANDOM_ACCESS_ITERATOR::value_type>
	static bool create(T_RANDOM_ACCESS_ITERATOR begin, T_RANDOM_ACCESS_ITERATOR end, UByteArrayAdapter & destination) {
		Static::ArrayCreator<T_VALUE_TYPE> creator(destination);
		for(;begin != end; ++begin) {
			creator.put( *begin );
		}
		creator.flush();
		return true;
	}
	
	///The order of the strings impacts the type
	template<typename T_RANDOM_ACCESS_CONTAINER, typename T_VALUE_TYPE = typename T_RANDOM_ACCESS_CONTAINER::value_type>
	static bool create(UByteArrayAdapter & destination, const T_RANDOM_ACCESS_CONTAINER & src) {
		return create<typename T_RANDOM_ACCESS_CONTAINER::const_iterator, T_VALUE_TYPE>(src.cbegin(), src.cend(), destination);
	}
	

};

class FrequencyStringTable: public StringTable {
public:
	FrequencyStringTable();
	FrequencyStringTable(const UByteArrayAdapter & data);
	FrequencyStringTable(UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag);
	FrequencyStringTable(UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag);
	FrequencyStringTable(const FrequencyStringTable & other);
	FrequencyStringTable & operator=(const FrequencyStringTable & other);
	virtual ~FrequencyStringTable();
	inline SizeType find(const std::string & value) const { return priv()->find(value);}
	inline bool count(const std::string & value) const { return find(value) != npos; }
};

class SortedStringTable: public StringTable {
public:
	SortedStringTable();
	SortedStringTable(const UByteArrayAdapter & data);
	SortedStringTable(UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag);
	SortedStringTable(UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag);
	SortedStringTable(const SortedStringTable & other);
	virtual ~SortedStringTable();
	SortedStringTable & operator=(SortedStringTable const &) = default;
	SizeType find(const std::string & value) const;
	std::pair<SizeType, SizeType> range(const std::string & prefix) const;
	inline bool count(const std::string & value) const { return find(value) != npos; }
};

}}//end namespace

#endif
