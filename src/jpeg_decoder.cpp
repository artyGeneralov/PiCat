#include "../include/jpeg.h"
#include <cstdlib>
#include <iostream>
#include "../include/bit_reader.h"

byte getNextSymbol(BitReader&, const HuffmanTable&);
bool decodeMCUComponent(BitReader&, int* const, int&, const HuffmanTable&, const HuffmanTable&);
void generateHuffmanCodes(HuffmanTable&);

MCU* decodeHuffmanData(JPEGImage* const jpeg) {
	const uint mcuRows = (jpeg->height + 7) / 8;
	const uint mcuColumns = (jpeg->width + 7) / 8;
	int prevDCCoefficients[3] = { 0 };
	std::cout << "jpegHeight: " << jpeg->height << " jpegWidth: " << jpeg->width << " mcuRows: " << mcuRows << " mcuCols: " << mcuColumns << "\n";
	MCU* mcus = new (std::nothrow) MCU[mcuRows * mcuColumns];
	if (mcus == nullptr) {
		std::cout << "Error: Decoder error, mcus are null\n";
		return nullptr;
	}

	for (uint i = 0; i < 4; ++i) {
		if (jpeg->huffmanDCTables[i].set) {
			generateHuffmanCodes(jpeg->huffmanDCTables[i]);
		}

		if (jpeg->huffmanACTables[i].set) {
			generateHuffmanCodes(jpeg->huffmanACTables[i]);
		}
	}
	
	BitReader bitReader(jpeg->huffmanData);
	for (uint i = 0; i < mcuRows * mcuColumns; ++i) {
		if (jpeg->restartInterval != 0 && i % jpeg->restartInterval == 0) {
			prevDCCoefficients[0] = 0;
			prevDCCoefficients[1] = 0;
			prevDCCoefficients[2] = 0;
			bitReader.align();
		}
		for (uint j = 0; j < jpeg->numComponents; ++j) {
			 
			if (!decodeMCUComponent(bitReader,
				(mcus[i])[j],
				prevDCCoefficients[j],
				jpeg->huffmanDCTables[jpeg->colorComponents[j].huffmanDCTableID],
				jpeg->huffmanACTables[jpeg->colorComponents[j].huffmanACTableID])) { // decodeMCUComponent processes a single channel of a single MCU
				delete[] mcus;
				return nullptr;
			}
		}
	}
	return mcus;
}

bool decodeMCUComponent(BitReader& br, int* const component, int& prevDC, const HuffmanTable& dcTable, const HuffmanTable& acTable) {
	// Get DC Value for this mcu component
	byte length = getNextSymbol(br, dcTable);
	if (length == (byte)-1) {
		std::cout << "Error: Invalid DC Value\n";
		return false;
	}
	if (length > 11) {
		std::cout << "Error: DC Coefficient can't be larger than 11\n";
	}
	int coefficient = br.readBits(length);
	if (coefficient == -1) {
		std::cout << "Error: invalid DC value\n";
		return false;
	}
	if (length != 0 && coefficient < (1 << (length - 1))) {
		coefficient -= (1 << length) - 1;
	}
	component[0] = coefficient + prevDC;
	prevDC = component[0];

	//// Get AC Values:
	uint i = 1;
	while (i < 64) {
		byte symbol = getNextSymbol(br, acTable);
		if (symbol == (byte)-1) {
			std::cout << "Error: Invalid AC value\n";
			return false;
		}
		if (symbol == 0x00) {
			for (; i < 64; ++i) {
				component[zigZagMap[i]] = 0;
			}
			return true;
		}
		byte zerosToSkip = symbol >> 4;
		byte coefficientLength = symbol & 0x0F;
		coefficient = 0;
		if (symbol == 0xF0) {
			zerosToSkip = 16;
		}

		if (i + zerosToSkip >= 64) {
			std::cout << "Error: zeros length exceeds MCU length\n";
			return false;
		}
		for (uint j = 0; j < zerosToSkip; ++j, ++i) {
			component[zigZagMap[i]] = 0;
		}
		if (coefficientLength > 10) {
			std::cout << "Error: AC coefficient length greater than 10 not allowed\n";
			return false;
		}
		if (coefficientLength != 0) {
			coefficient = br.readBits(coefficientLength);
			if (coefficient == -1) {
				std::cout << "Error: AC value invalid\n";
				return false;
			}
			if (coefficient < (1 << (coefficientLength - 1))) {
				coefficient -= (1 << coefficientLength) - 1;
			}
			component[zigZagMap[i]] = coefficient;
			i += 1;
		}
	}
	return true;
}


byte getNextSymbol(BitReader& br, const HuffmanTable& table) {
	uint currentCode = 0;
	for (uint i = 0; i < 16; ++i) {
		int bit = br.readNextBit();
		if (bit == -1){
			return -1; // 255 but 0xFF is not a valid symbol
		}
		currentCode = (currentCode << 1) | bit;
		for (uint j = table.offsets[i]; j < table.offsets[i + 1]; ++j) {
			if (currentCode == table.codes[j]) {
				return table.symbols[j];
			}
		}
	}
	return -1;
}

void generateHuffmanCodes(HuffmanTable& table) {
	uint code = 0;
	for (uint i = 0; i < 16; ++i) {
		for (uint j = table.offsets[i]; j < table.offsets[i + 1]; ++j) {
			table.codes[j] = code;
			code += 1;
		}
		code <<= 1; // append 0 to right end of code candidate
	}
}
