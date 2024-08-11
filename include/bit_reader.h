#ifndef BIT_READER_H
#define BIT_READER_H
#include <vector>
#include "utils.h"

class BitReader {
public:
	BitReader(const std::vector<byte>& data);
	int readNextBit();
	int readBits(const uint length);
	void align();
private:
	const std::vector<byte>& data;
	size_t byteIndex;
	int bitIndex;
};

#endif