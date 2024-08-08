#include "../../include/utils.h"

// Writing in little endian

void putLong(std::ofstream& outFile, const uint data) {
	outFile.put((data >> 0) & 0xFF);
	outFile.put((data >> 8) & 0xFF);
	outFile.put((data >> 16) & 0xFF);
	outFile.put((data >> 24) & 0xFF);
}

void putShort(std::ofstream& outFile, const uint data) {
	outFile.put((data >> 0) & 0xFF);
	outFile.put((data >> 8) & 0xFF);
}