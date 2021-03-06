#include <vector>
#include <sserialize/iterator/MultiBitBackInserter.h>
#include <sserialize/iterator/MultiBitIterator.h>
#include <sserialize/algorithm/utilfuncs.h>
#include "TestBase.h"

using namespace sserialize;

void print(std::stringstream & /*dest*/) {}

template<typename T, typename ... Args>
void print(std::stringstream & dest, T t, Args ... args) {
	dest << t;
	print(dest, args...);
}


template<typename ... Args>
std::string printToString(Args ... args) {
	std::stringstream  ss;
	print(ss, args...);
	return ss.str();
}

template<int NumberOfRuns, int TestDataLength>
class MultiBitIteratorTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( MultiBitIteratorTest );
CPPUNIT_TEST( testSpecialEquality );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST( testRunofSameBits );
CPPUNIT_TEST_SUITE_END();
private:
	std::vector< std::pair<uint64_t, uint8_t> > createTestData() {
		std::vector< std::pair<uint64_t, uint8_t> > ret;
		for(int i = 0; i < TestDataLength; ++i) {
			uint64_t num = random();
			uint8_t bits = rand() % 64 + 1;
			num = num & sserialize::createMask64(bits);
			ret.push_back( std::pair<uint64_t, uint8_t>(num, bits) );
		}
		return ret;
	}
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testSpecialEquality() {
		typedef std::pair<uint64_t, uint8_t> DataFrameType;
		std::vector< std::pair<uint64_t, uint8_t> > testData;
		std::vector<uint8_t> dataStore;
		
		testData.push_back( DataFrameType(0x67, 7));
		testData.push_back(DataFrameType(0x60000007, 52));
		testData.push_back(DataFrameType(0x70000003, 64));
		
		
		
		UByteArrayAdapter data(&dataStore, false);
		MultiBitBackInserter backInserter(data);
		for(auto & x : testData) {
			backInserter.push_back(x.first, x.second);
		}
		backInserter.flush();
		data = backInserter.data();
		MultiBitIterator it(data);
		
		for(std::size_t j = 0; j < testData.size(); ++j) {
			std::string str = printToString("pos ", j);
			uint64_t rv = testData[j].first;
			uint64_t v = it.get64(testData[j].second);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(str.c_str(), rv, v);
			it += testData[j].second;
		}
	}
	
	void testEquality() {
		for(int i = 0; i < NumberOfRuns; ++i) {
			std::vector< std::pair<uint64_t, uint8_t> > testData = createTestData();
			std::vector<uint8_t> dataStore;
			UByteArrayAdapter data(&dataStore, false);
			MultiBitBackInserter backInserter(data);
			for(auto & x : testData) {
				backInserter.push_back(x.first, x.second);
			}
			backInserter.flush();
			data = backInserter.data();
			MultiBitIterator it(data);
			
			for(std::size_t j = 0; j < testData.size(); ++j) {
				std::string str = printToString("run ", i, " pos ", j);
				uint64_t rv = testData[j].first;
				uint64_t v = it.get64(testData[j].second);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(str.c_str(), rv, v);
				it += testData[j].second;
			}
		}
	}
	
	void testRunofSameBits() {
		UByteArrayAdapter data(0, MM_PROGRAM_MEMORY);
		for(std::size_t bits(1); bits < 64; ++bits) {
			uint64_t mask = createMask64(bits);
			for(std::size_t i(2); i < 1024; ++i) {
				data.resetPtrs();
				MultiBitBackInserter backInserter(data);
				for(uint64_t j(0); j < i; ++j) {
					backInserter.push_back(j & mask, bits);
				}
				backInserter.flush();
				MultiBitIterator mbit(data);
				for(uint64_t j(0); j < i; ++j) {
					uint64_t v = mbit.get<uint64_t>(bits);
					CPPUNIT_ASSERT_EQUAL_MESSAGE("bits=" + std::to_string(bits) + ", i=" + std::to_string(i) + ", j=" + std::to_string(j), j & mask, v);
					mbit += bits;
				}
			}
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand(0);
	srandom( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( MultiBitIteratorTest<10, 7>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 63>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 127>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 234>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 554>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 1011>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 2022>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 5034>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 15034>::suite() );
	runner.addTest( MultiBitIteratorTest<10, 235034>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
