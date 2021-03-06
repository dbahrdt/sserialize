#include <sserialize/utility/Bitpacking.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
	
std::unique_ptr<BitpackingInterface> BitpackingInterface::instance(uint32_t bpn) {
#define C(__BPN) case __BPN: return std::unique_ptr<BitpackingInterface>( new Bitpacking<__BPN>() );
	switch (bpn) {
	C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31); C(32); C(33); C(34); C(35); C(36); C(37); C(38); C(39); C(40);
	C(41); C(42); C(43); C(44); C(45); C(46); C(47); C(48); C(49); C(50);
	C(51); C(52); C(53); C(54); C(55); C(56);
	C(64);
	default:
		throw sserialize::UnsupportedFeatureException("BitpackingInterface: unsupported block bits: " + std::to_string(bpn));
	};
#undef C
}

}//end namespace sserialize
