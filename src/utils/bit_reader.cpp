#include "../../include/bit_reader.h"
BitReader::BitReader(const std::vector<byte>& data) :
	data(data), byteIndex(0), bitIndex(7) {}

int BitReader::readNextBit() {
	if (byteIndex > data.size()) {
		return -1;
	}

	int bit = (data[byteIndex] >> bitIndex) & 1;
	--bitIndex;
	if (bitIndex < 0) {
		bitIndex = 7;
		++byteIndex;
	}
	return bit;
}

int BitReader::readBits(const uint length) {
	int bits = 0;
	for (uint i = 0; i < length; ++i){
		int bit = readNextBit();
		if (bit == -1) {
			bits = -1;
			break;
		}
		bits = (bits << 1) | bit;
	}
	return bits;
}