#ifndef SSERIALIZE_VERSION_CHECKER_H
#define SSERIALIZE_VERSION_CHECKER_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/constants.h>

namespace sserialize {

///A simple hack to allow version checking in the initializer list
struct VersionChecker {
	inline static const sserialize::UByteArrayAdapter & 
	check(const sserialize::UByteArrayAdapter & d, uint32_t want, uint32_t have, const char * msg) {
		if (UNLIKELY_BRANCH(want != have)) {
			throw sserialize::VersionMissMatchException(std::string(msg), want, have);
		}
		return d;
	}

	inline static sserialize::UByteArrayAdapter & 
	check(sserialize::UByteArrayAdapter & d, uint32_t want, uint32_t have, const char * msg) {
		if (UNLIKELY_BRANCH(want != have)) {
			throw sserialize::VersionMissMatchException(std::string(msg), want, have);
		}
		return d;
	}
	
	//check version which is at the d[0]
	inline static sserialize::UByteArrayAdapter
	check(sserialize::UByteArrayAdapter const & d, uint8_t want, const char * msg) {
		uint8_t have = d.getUint8(0);
		if (UNLIKELY_BRANCH(want != have)) {
			throw sserialize::VersionMissMatchException(std::string(msg), want, have);
		}
		return d+1;
	}
	
	//check version which is at the d[0], increment d by 1
	static sserialize::UByteArrayAdapter &
	checkNConsume(sserialize::UByteArrayAdapter & d, uint8_t want, const char * msg) {
		uint8_t have = d.getUint8(0);
		if (UNLIKELY_BRANCH(want != have)) {
			throw sserialize::VersionMissMatchException(std::string(msg), want, have);
		}
		d += 1;
		return d;
	}
};


}//end namespace

#endif
