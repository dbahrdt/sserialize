#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <vector>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/DynamicBitSet.h>
#include "datacreationfuncs.h"

using namespace sserialize;


std::set<uint32_t> myCreateNumbers(uint32_t count, uint32_t maxNum) {
	std::set<uint32_t> ret;
	uint32_t rndNum;
	for(uint32_t i = 0; i < count; i++) {
		rndNum = (double)rand()/RAND_MAX * maxNum;
		ret.insert(rndNum);
	}
	return ret;
}

DynamicBitSet createBitSet(const std::set<uint32_t> & src) {
	UByteArrayAdapter data(UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
	DynamicBitSet bitSet(data);
	for(std::set<uint32_t>::const_iterator it(src.begin()); it != src.end(); ++it) {
		bitSet.set(*it);
	}
	return bitSet;
}

class DynamicBitSetTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( DynamicBitSetTest );
CPPUNIT_TEST( testRandomEquality );
CPPUNIT_TEST( testIndexCreation );
CPPUNIT_TEST( testIntersection );
CPPUNIT_TEST( testMerge );
CPPUNIT_TEST( testDifference );
CPPUNIT_TEST( testSymDiff );
CPPUNIT_TEST_SUITE_END();
private:
	int testCount;
public:
	virtual void setUp() { testCount = 1; }
	virtual void tearDown() {}
	
	void testRandomEquality() {
		srand(0);
		uint32_t setCount = testCount;

		for(size_t i = 0; i < setCount; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			DynamicBitSet bitSet(dest);
			
			uint32_t maxNum = *realValues.rbegin();
			
			for(std::set<uint32_t>::const_iterator it = realValues.begin(); it != realValues.end(); ++it) {
				bitSet.set(*it);
			}
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bit set size", (uint32_t)realValues.size(), (uint32_t)bitSet.size());
			
			for(uint32_t j = 0; j <= maxNum+16 || j < bitSet.data().size()*8; ++j) {
				std::stringstream ss;
				ss << "id " << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), (realValues.count(j) > 0), bitSet.isSet(j));
			}
		}
	}
	
	void testIndexCreation() {
		srand(0);
		uint32_t setCount = testCount;

		for(size_t i = 0; i < setCount; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			DynamicBitSet bitSet(dest);
			
			for(std::set<uint32_t>::const_iterator it = realValues.begin(); it != realValues.end(); ++it) {
				bitSet.set(*it);
			}
			ItemIndex idx(bitSet.toIndex(ItemIndex::T_STL_VECTOR));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t) realValues.size(),  idx.size());
			
			CPPUNIT_ASSERT_MESSAGE("idx equality", (realValues == idx));
		}
	}

	void testIntersection() {
		for(int i = 0; i < testCount; ++i) {
			std::set<uint32_t> a, b, c;
			createOverLappingSets(a, b, 0xFFFF, 1024, 1024, 512);
			DynamicBitSet bitSetA(createBitSet(a));
			DynamicBitSet bitSetB(createBitSet(b));
			
			DynamicBitSet bitSetOp = bitSetA & bitSetB;
			std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bit set size", (uint32_t)c.size(), (uint32_t)bitSetOp.size());
			
			ItemIndex idx = bitSetOp.toIndex(ItemIndex::T_STL_VECTOR);
			
			CPPUNIT_ASSERT_MESSAGE("idx equality", (c == idx));
		}
	}
	
	void testMerge() {
		for(int i = 0; i < testCount; ++i) {
			std::set<uint32_t> a, b, c;
			createOverLappingSets(a, b, 0xFFFF, 1024, 1024, 512);
			DynamicBitSet bitSetA(createBitSet(a));
			DynamicBitSet bitSetB(createBitSet(b));
			
			DynamicBitSet bitSetOp = bitSetA | bitSetB;
			std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bit set size", (uint32_t)c.size(), (uint32_t)bitSetOp.size());
			
			ItemIndex idx = bitSetOp.toIndex(ItemIndex::T_STL_VECTOR);
			
			CPPUNIT_ASSERT_MESSAGE("idx equality", (c == idx));
		}
	}
	
	void testDifference() {
		for(int i = 0; i < testCount; ++i) {
			std::set<uint32_t> a, b, c;
			createOverLappingSets(a, b, 0xFFFF, 1024, 1024, 512);
			DynamicBitSet bitSetA(createBitSet(a));
			DynamicBitSet bitSetB(createBitSet(b));
			
			DynamicBitSet bitSetOp = bitSetA - bitSetB;
			std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bit set size", (uint32_t)c.size(), (uint32_t)bitSetOp.size());
			
			ItemIndex idx = bitSetOp.toIndex(ItemIndex::T_STL_VECTOR);
			
			CPPUNIT_ASSERT_MESSAGE("idx equality", (c == idx));
		}
	}
	
	void testSymDiff() {
		for(int i = 0; i < testCount; ++i) {
			std::set<uint32_t> a, b, c;
			createOverLappingSets(a, b, 0xFFFF, 1024, 1024, 512);
			DynamicBitSet bitSetA(createBitSet(a));
			DynamicBitSet bitSetB(createBitSet(b));
			
			DynamicBitSet bitSetOp = bitSetA ^ bitSetB;
			std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bit set size", (uint32_t)c.size(), (uint32_t)bitSetOp.size());
			
			ItemIndex idx = bitSetOp.toIndex(ItemIndex::T_STL_VECTOR);
			
			CPPUNIT_ASSERT_MESSAGE("idx equality", (c == idx));
		}
	}
	
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  DynamicBitSetTest::suite() );
	runner.run();
	return 0;
}