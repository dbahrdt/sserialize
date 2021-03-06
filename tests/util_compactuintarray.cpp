#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/algorithm/utilmath.h>
#include <sserialize/utility/log.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "TestBase.h"

using namespace sserialize;


uint32_t testLen = 0xFFFFF;

class TestCompactUintArray: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TestCompactUintArray );
CPPUNIT_TEST( createAutoBitsTest );
CPPUNIT_TEST( createManuBitsTest );
CPPUNIT_TEST( boundedTest );
CPPUNIT_TEST( veryLargeTest );
CPPUNIT_TEST( specialLargeTest );
CPPUNIT_TEST_SUITE_END();
private:
	std::vector<uint64_t> compSrcArrays[64];
public:

	virtual void setUp() {
		//Fill the first
		srand( 0 );
		uint64_t rndNum;
		uint64_t mask;
		for(int j=0; j < 64; ++j) {
			compSrcArrays[j].push_back(createMask64(j+1));
		}
		for(uint32_t i = 0; i < testLen; i++) {
			rndNum = static_cast<uint64_t>(rand()) << 32 | rand();
			for(int j=0; j < 64; j++) {
				mask = createMask64(j+1);
				compSrcArrays[j].push_back(rndNum & mask);
			}
		}
	}
	
	virtual void tearDown() {
		for(auto & x : compSrcArrays) {
			x = std::decay_t<decltype(x)>();
		}
	}

	void createAutoBitsTest() {
		for(uint32_t bits = 0; bits < 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			uint8_t createBits = CompactUintArray::create(compSrcArrays[bits], d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("create bits", bits+1,static_cast<uint32_t>(createBits));
			d.resetPtrs();
			CompactUintArray carr(d, createBits);
			for(uint32_t i = 0, s = (uint32_t) compSrcArrays[bits].size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("bits=",bits," at ", i), compSrcArrays[bits][i], carr.at64(i));
			}
		}
	}
	
	void createManuBitsTest() {
		for(uint32_t bits = 0; bits < 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			CompactUintArray::create(compSrcArrays[bits], d, bits+1);
			d.resetPtrs();
			CompactUintArray carr(d, bits+1);
			for(uint32_t i = 0, s = (uint32_t) compSrcArrays[bits].size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), compSrcArrays[bits][i], carr.at64(i));
			}
		}
	}

	void boundedTest() {
		for(uint32_t bits = 0; bits < 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			BoundedCompactUintArray::create(compSrcArrays[bits], d);
			d.resetPtrs();
			BoundedCompactUintArray bcarr(d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("for bits=", bits+1), compSrcArrays[bits].size(), (std::size_t)bcarr.size());
			for(uint32_t i = 0, s = (uint32_t) compSrcArrays[bits].size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), compSrcArrays[bits][i], bcarr.at64(i));
			}
		}
	}
	
	void veryLargeTest() {
		for(uint32_t bits = 1; bits <= 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			auto count = narrow_check<CompactUintArray::value_type>(uint64_t(std::numeric_limits<uint32_t>::max())+1024);
			uint64_t mask = createMask64(bits);
			CompactUintArray carr(d, bits);
			carr.reserve(count);
			sserialize::RangeGenerator<uint64_t> rg(0, count);
			{
				auto rit = rg.rbegin();
				auto rend = rg.rend();
				for(std::size_t i(0); i < 10*1000 && rit != rend; ++i, ++rit) {
					auto x = *rit;
					CPPUNIT_ASSERT_EQUAL_MESSAGE("setting", x & mask, carr.set64(x, x & mask));
				}
			}
			{
				auto rit = rg.rbegin();
				auto rend = rg.rend();
				for(std::size_t i(0); i < 10*1000 && rit != rend; ++i, ++rit) {
					auto x = *rit;
					CPPUNIT_ASSERT_EQUAL_MESSAGE("getting", x & mask, carr.at64(x));
				}
			}
		}
	}
	
	void specialLargeTest() {
		uint8_t bits = 31;
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			uint32_t count = 1500*1000*1000;
			uint64_t mask = createMask64(bits);
			CompactUintArray carr(d, bits);
			carr.reserve(count);
			sserialize::RangeGenerator<uint32_t> rg(0, count);
			for(uint64_t x : rg) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("setting", x & mask, carr.set64((uint32_t) x, x & mask));
			}
			for(uint64_t x : rg) {
				if ((x & mask) != carr.at64((uint32_t) x)) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE("getting", x & mask, carr.at64((uint32_t) x));
				}
			}
	
	}
	
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestCompactUintArray::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
