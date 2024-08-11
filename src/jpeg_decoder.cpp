#include "../include/jpeg.h"
#include <cstdlib>
#include <iostream>
#include <cmath>
#include "../include/bit_reader.h"

byte getNextSymbol(BitReader&, const HuffmanTable&);
bool decodeMCUComponent(BitReader&, int* const, int&, const HuffmanTable&, const HuffmanTable&);
void generateHuffmanCodes(HuffmanTable&);
void dequantizeComponent(const QuantizationTable&, int* const);
void inverseDCTComp(int* const);
void clampBetween(int&, const int&, const int&);
void convertMCU_ToRGB(MCU&);

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
			#pragma warning(push)
			#pragma warning(disable: 6385)
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

void dequantize(const JPEGImage* const jpeg, MCU* const mcus) {
	const uint mcuRows = (jpeg->height + 7) / 8;
	const uint mcuCols = (jpeg->width + 7) / 8;
	for (uint i = 0; i < mcuRows * mcuCols; ++i) {
		for (uint j = 0; j < jpeg->numComponents; ++j) {
			dequantizeComponent(jpeg->quantizationTables[jpeg->colorComponents[j].quantizationTableID], (mcus[i])[j]);
		}
	}
}

void dequantizeComponent(const QuantizationTable& qt, int* const component) {
	for (uint i = 0; i < 64; ++i) {
		component[i] *= qt.table[i];
	}
}


void inverseDCT(const JPEGImage* const jpeg, MCU* const mcus) {


	const uint mcuRows = (jpeg->height + 7) / 8;
	const uint mcuCols = (jpeg->width + 7) / 8;
	for (uint i = 0; i < mcuRows * mcuCols; ++i) {
		for (uint j = 0; j < jpeg->numComponents; ++j) {
			inverseDCTComp((mcus[i])[j]);
		}
	}
}

void inverseDCTComp(int* const component) {
	//AAN:
	// input: 0, 4, 2, 6, 5, 1, 7, 3
	float temp[64];

	for (uint i = 0; i < 8; ++i) {
		const float g0 = component[0 * 8 + i] * s0;
		const float g1 = component[4 * 8 + i] * s4;
		const float g2 = component[2 * 8 + i] * s2;
		const float g3 = component[6 * 8 + i] * s6;
		const float g4 = component[5 * 8 + i] * s5;
		const float g5 = component[1 * 8 + i] * s1;
		const float g6 = component[7 * 8 + i] * s7;
		const float g7 = component[3 * 8 + i] * s3;

		const float f0 = g0;
		const float f1 = g1;
		const float f2 = g2;
		const float f3 = g3;
		const float f4 = g4 - g7;
		const float f5 = g5 + g6;
		const float f6 = g5 - g6;
		const float f7 = g4 + g7;

		const float e0 = f0;
		const float e1 = f1;
		const float e2 = f2 - f3;
		const float e3 = f2 + f3;
		const float e4 = f4;
		const float e5 = f5 - f7;
		const float e6 = f6;
		const float e7 = f5 + f7;
		const float e8 = f4 + f6;

		const float d0 = e0;
		const float d1 = e1;
		const float d2 = e2 * m1;
		const float d3 = e3;
		const float d4 = e4 * m2;
		const float d5 = e5 * m3;
		const float d6 = e6 * m4;
		const float d7 = e7;
		const float d8 = e8 * m5;

		const float c0 = d0 + d1;
		const float c1 = d0 - d1;
		const float c2 = d2 - d3;
		const float c3 = d3;
		const float c4 = d4 + d8;
		const float c5 = d5 + d7;
		const float c6 = d6 - d8;
		const float c7 = d7;
		const float c8 = c5 - c6;

		const float b0 = c0 + c3;
		const float b1 = c1 + c2;
		const float b2 = c1 - c2;
		const float b3 = c0 - c3;
		const float b4 = c4 - c8;
		const float b5 = c8;
		const float b6 = c6 - c7;
		const float b7 = c7;

		temp[0 * 8 + i] = b0 + b7;
		temp[1 * 8 + i] = b1 + b6;
		temp[2 * 8 + i] = b2 + b5;
		temp[3 * 8 + i] = b3 + b4;
		temp[4 * 8 + i] = b3 - b4;
		temp[5 * 8 + i] = b2 - b5;
		temp[6 * 8 + i] = b1 - b6;
		temp[7 * 8 + i] = b0 - b7;
	}
	for (uint i = 0; i < 8; ++i) {
		const float g0 = temp[i * 8 + 0] * s0;
		const float g1 = temp[i * 8 + 4] * s4;
		const float g2 = temp[i * 8 + 2] * s2;
		const float g3 = temp[i * 8 + 6] * s6;
		const float g4 = temp[i * 8 + 5] * s5;
		const float g5 = temp[i * 8 + 1] * s1;
		const float g6 = temp[i * 8 + 7] * s7;
		const float g7 = temp[i * 8 + 3] * s3;

		const float f0 = g0;
		const float f1 = g1;
		const float f2 = g2;
		const float f3 = g3;
		const float f4 = g4 - g7;
		const float f5 = g5 + g6;
		const float f6 = g5 - g6;
		const float f7 = g4 + g7;

		const float e0 = f0;
		const float e1 = f1;
		const float e2 = f2 - f3;
		const float e3 = f2 + f3;
		const float e4 = f4;
		const float e5 = f5 - f7;
		const float e6 = f6;
		const float e7 = f5 + f7;
		const float e8 = f4 + f6;

		const float d0 = e0;
		const float d1 = e1;
		const float d2 = e2 * m1;
		const float d3 = e3;
		const float d4 = e4 * m2;
		const float d5 = e5 * m3;
		const float d6 = e6 * m4;
		const float d7 = e7;
		const float d8 = e8 * m5;

		const float c0 = d0 + d1;
		const float c1 = d0 - d1;
		const float c2 = d2 - d3;
		const float c3 = d3;
		const float c4 = d4 + d8;
		const float c5 = d5 + d7;
		const float c6 = d6 - d8;
		const float c7 = d7;
		const float c8 = c5 - c6;

		const float b0 = c0 + c3;
		const float b1 = c1 + c2;
		const float b2 = c1 - c2;
		const float b3 = c0 - c3;
		const float b4 = c4 - c8;
		const float b5 = c8;
		const float b6 = c6 - c7;
		const float b7 = c7;

		component[i * 8 + 0] = b0 + b7 + 0.5f;
		component[i * 8 + 1] = b1 + b6 + 0.5f;
		component[i * 8 + 2] = b2 + b5 + 0.5f;
		component[i * 8 + 3] = b3 + b4 + 0.5f;
		component[i * 8 + 4] = b3 - b4 + 0.5f;
		component[i * 8 + 5] = b2 - b5 + 0.5f;
		component[i * 8 + 6] = b1 - b6 + 0.5f;
		component[i * 8 + 7] = b0 - b7 + 0.5f;
	}
}

void convertToRGB(const JPEGImage* jpeg, MCU* const mcus) {
	const uint mcuRows = (jpeg->height + 7) / 8;
	const uint mcuCols = (jpeg->width + 7) / 8;
	for (uint i = 0; i < mcuRows * mcuCols; ++i) {
		convertMCU_ToRGB(mcus[i]);
	}
}

void convertMCU_ToRGB(MCU& mcu) {
	for (uint i = 0; i < 64; ++i) {
		int r = mcu.y[i] + 1.402f * mcu.cr[i] + 128;
		int g = mcu.y[i] + 0.344f * mcu.cb[i] - 0.714f * mcu.cr[i] + 128;
		int b = mcu.y[i] + 1.772f * mcu.cb[i] + 128;
		clampBetween(r, 0, 255);
		clampBetween(g, 0, 255);
		clampBetween(b, 0, 255);
		mcu.r[i] = r;
		mcu.g[i] = g;
		mcu.b[i] = b;
	}
}

void clampBetween(int& n, const int& start, const int& end) {
	if (n < start) {
		n = start;
	}
	else if (n > end) {
		n = end;
	}
}