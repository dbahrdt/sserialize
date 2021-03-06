#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <iostream>

int main(int, char ** argv) {
	sserialize::UByteArrayAdapter::setTempFilePrefix(argv[0]);
	uint64_t size = atoll(argv[1]);
	sserialize::MmappedMemory<int> mem(size, sserialize::MM_FILEBASED);
	std::cout << "Mapped " << size << " ints" << std::endl;
	exit(0);
	return 0;
}