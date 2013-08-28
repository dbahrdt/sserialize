#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/utilmath.h>
#include <sserialize/utility/log.h>

int TestCount = 10000;

class PackFunctions: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( PackFunctions );
CPPUNIT_TEST( test16 );
CPPUNIT_TEST( test24 );
CPPUNIT_TEST( test32 );
CPPUNIT_TEST( test40 );
CPPUNIT_TEST( test64 );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void test16() {
		uint32_t mask = sserialize::createMask(15);
		uint8_t buf[16];
		memset(buf, 1, 16);
		for(int i=0; i < TestCount; ++i) {
			uint16_t num = rand() & mask;
			int16_t sNum = num;
			sserialize::pack_uint16_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u16", num, sserialize::unPack_uint16_t(buf));
			
			sserialize::p_s16(static_cast<int16_t>(num), buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s16+", sNum, sserialize::up_s16(buf));
			
			sNum = -sNum;
			sserialize::p_s16(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s16-", sNum, sserialize::up_s16(buf));
		}
		
		for(uint8_t bpn = 0; bpn <= 16; ++bpn) {
			uint16_t num = sserialize::createMask(bpn);
			int16_t sNum = num;
			sserialize::pack_uint16_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u16", num, sserialize::unPack_uint16_t(buf));
			
			if (bpn < 16) {
				sserialize::p_s16(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s16+", sNum, sserialize::up_s16(buf));
			
				sNum = -sNum;
				sserialize::p_s16(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s16-", sNum, sserialize::up_s16(buf));
			}
		}
	}
	
	void test24() {
		uint32_t mask = sserialize::createMask(23);
		uint8_t buf[16];
		memset(buf, 1, 16);
		for(int i=0; i < TestCount; ++i) {
			uint32_t num = rand() & mask;
			int32_t sNum = num;
			sserialize::pack_uint24_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u24", num, sserialize::unPack_uint24_t(buf));
			
			sserialize::p_s24(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s24+", sNum, sserialize::up_s24(buf));
			
			sNum = -sNum;
			sserialize::p_s24(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s24-", sNum, sserialize::up_s24(buf));
		}
		for(uint8_t bpn = 0; bpn <= 24; ++bpn) {
			uint32_t num = sserialize::createMask(bpn);
			int32_t sNum = num;
			sserialize::pack_uint24_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u24", num, sserialize::unPack_uint24_t(buf));
			
			if (bpn < 24) {
				sserialize::p_s24(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s24+", sNum, sserialize::up_s24(buf));
			
				sNum = -sNum;
				sserialize::p_s24(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s24-", sNum, sserialize::up_s24(buf));
			}
		}
	}
	
	void test32() {
		uint32_t mask = sserialize::createMask(31);
		uint8_t buf[16];
		memset(buf, 1, 16);
		for(int i=0; i < TestCount; ++i) {
			uint32_t num = rand() & mask;
			int32_t sNum = num;
			sserialize::pack_uint32_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u32", num, sserialize::unPack_uint32_t(buf));
			
			sserialize::pack_int32_t(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s32+", sNum, sserialize::unPack_int32_t(buf));
			
			sNum = -sNum;
			sserialize::pack_int32_t(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s32-", sNum, sserialize::unPack_int32_t(buf));
		}
		
		for(uint8_t bpn = 0; bpn <= 32; ++bpn) {
			int realLen = bpn/7 + ((bpn % 7 || bpn == 0) ? 1 : 0);
			uint32_t num = sserialize::createMask(bpn);
			int32_t sNum = num;
			sserialize::pack_uint32_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u32", num, sserialize::unPack_uint32_t(buf));
			
			int len = sserialize::p_v<uint32_t>(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u/p_v32 len", realLen, len);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u/p_v32", sserialize::up_v<uint32_t>(buf, 0), num);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("vl_size ", "bpn=", bpn, ";num=", num), len, sserialize::vl_pack_uint32_t_size(num));


			if (bpn < 32) {
				sserialize::pack_int32_t(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s32+", sNum, sserialize::unPack_int32_t(buf));
			
				sNum = -sNum;
				sserialize::pack_int32_t(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s32-", sNum, sserialize::unPack_int32_t(buf));
				
				int len = sserialize::p_v<int32_t>(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("u/p_sv32", sserialize::up_v<int32_t>(buf, 0), sNum);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("vl_size ", "bpn=", bpn, ";num=", sNum), len, sserialize::vl_pack_int32_t_size(sNum));
			}
		}
		
	}

	void test40() {
		uint64_t mask = sserialize::createMask64(39);
		uint8_t buf[16];
		memset(buf, 1, 16);
		for(int i=0; i < TestCount; ++i) {
			uint64_t num = ((static_cast<uint64_t>(rand()) << 8) | rand()) & mask;
			int64_t sNum = num;
			sserialize::pack_uint40_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u40", num, sserialize::unPack_uint40_t(buf));
			
			sserialize::pack_int40_t(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s40+", sNum, sserialize::unPack_int40_t(buf));
			
			sNum = -sNum;
			sserialize::pack_int40_t(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s40-", sNum, sserialize::unPack_int40_t(buf));
		}
	}
	
	void test64() {
		uint64_t mask = sserialize::createMask64(63);
		uint8_t buf[16];
		memset(buf, 1, 16);
		for(int i=0; i < TestCount; ++i) {
			uint64_t num = ( (static_cast<uint64_t>(rand()) << 32) | rand()) & mask;
			int64_t sNum = num;
			sserialize::pack_uint64_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u64", num, sserialize::unPack_uint64_t(buf));
			
			sserialize::pack_int64_t(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s64+", sNum, sserialize::unPack_int64_t(buf));
			
			sNum = -sNum;
			sserialize::pack_int64_t(sNum, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("s64-", sNum, sserialize::unPack_int64_t(buf));
		}
		
		for(uint8_t bpn = 0; bpn <= 64; ++bpn) {
			int realLen = bpn/7 + ((bpn % 7 || bpn == 0) ? 1 : 0);
			uint64_t num = sserialize::createMask64(bpn);
			int64_t sNum = num;
			sserialize::pack_uint64_t(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u64", num, sserialize::unPack_uint64_t(buf));
			
			int len = sserialize::p_v<uint64_t>(num, buf);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u/p_v64 len", realLen, len);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("u/p_v64", sserialize::up_v<uint64_t>(buf, 0), num);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("vl_size ", "bpn=", bpn, ";num=", num), len, sserialize::psize_v<uint64_t>(num));


			if (bpn < 64) {
				sserialize::pack_int64_t(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s64+", sNum, sserialize::unPack_int64_t(buf));
			
				sNum = -sNum;
				sserialize::pack_int64_t(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("s64-", sNum, sserialize::unPack_int64_t(buf));
				
				int len = sserialize::p_v<int64_t>(sNum, buf);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("pack/unpack", sserialize::up_v<int64_t>(buf, 0), sNum);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("vl_size ", "bpn=", bpn, ";num=", sNum), len, sserialize::psize_v<int64_t>(sNum));
			}
		}
		
	}
	
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  PackFunctions::suite() );
	runner.run();
	return 0;
}