#include "../include/jpeg.h"
#include <iostream>
#include <fstream>
#include "../include/error_handler.h"


void parseQT(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing DQT Marker\n";
	int length = (inFile.get() << 8) | inFile.get();
	length -= 2;
	while (length > 0) {
		byte tableInfo = inFile.get(); // lower nibble holds table id, upper nibble holds the amount of bits
		length -= 1;
		byte tableID = tableInfo & 0x0F;
		if (tableID > 3) {
			ErrorHandler::logJPEGError("Error: Invalid Quatization Table ID " + std::to_string((uint)tableID) + "\n", jpeg->isValid);
			return;
		}
		jpeg->quantizationTables[tableID].set = true;
		if (tableInfo >> 4 != 0) { // 16-bit quantization table
			for (uint i = 0; i < 64; ++i) {
				jpeg->quantizationTables[tableID].table[zigZagMap[i]] = (inFile.get() << 8) | inFile.get();
			}
			length -= 128;
		}
		else { // 8-bit quantization table
			for (uint i = 0; i < 64; ++i) {
				jpeg->quantizationTables[tableID].table[zigZagMap[i]] = inFile.get();
			}
			length -= 64;
		}
	}
	if (length != 0) {
		ErrorHandler::logJPEGError("Error: DQT Marker invalid size\n", jpeg->isValid);
	}
}

void parseAPPN(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing APPN Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	for (uint i = 0; i < length - 2; ++i) {
		inFile.get();
	}
}

void parseCOM(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing COM Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	for (uint i = 0; i < length - 2; ++i) {
		inFile.get();
	}
}

void parseSOF(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing SOF Marker\n";
	if (jpeg->numComponents != 0) {
		ErrorHandler::logJPEGError("Error: Duplicate SOF Markers\n", jpeg->isValid);
		return;
	}
	uint length = (inFile.get() << 8) | inFile.get();
	byte precision = inFile.get();
	if (precision != 8) {
		ErrorHandler::logJPEGError("Error: Invalid precision. Must be 8, received " + std::to_string((uint)precision) + "\n", jpeg->isValid);
		return;
	}

	jpeg->height = (inFile.get() << 8) | inFile.get();
	jpeg->width = (inFile.get() << 8) | inFile.get();
	if (jpeg->height == 0 || jpeg->width == 0){
		ErrorHandler::logJPEGError("Error: Invalid dimensions\n", jpeg->isValid);
		return;
	}
	jpeg->numComponents = inFile.get();
	if (jpeg->numComponents != 3 && jpeg->numComponents != 1) {
		ErrorHandler::logJPEGError("Error: Invalid number of components", jpeg->isValid);
		return;
	}
	for (uint i = 0; i < jpeg->numComponents; ++i) {
		byte componentID = inFile.get();
		// Component ID can be 1, 2 or 3 in YCrCb Color mode
		// In rare cases ID can be 0, 1 or 2, So we force it to the range 1, 2 and 3
		if (componentID == 0) {
			jpeg->zeroBased = true;
		}
		if (jpeg->zeroBased == true) {
			componentID += 1;
		}
		if (componentID == 4 || componentID == 5) {
			ErrorHandler::logJPEGError("Error: YIQ Color mode not supported\n", jpeg->isValid); // TODO: Support YIQ Color mode
			return;
		}
		if (componentID == 0 || componentID > 3) {
			ErrorHandler::logJPEGError("Error: Invalid Component IDs\n", jpeg->isValid);
			return;
		}
		ColorComponent* component = &jpeg->colorComponents[componentID - 1];
		if (component->used) {
			ErrorHandler::logJPEGError("Error: Duplicate component definition for component: " + std::to_string((uint)componentID) + "\n", jpeg->isValid);
			return;
		}
		component->used = true;
		byte samplingFactor = inFile.get();
		component->componentID = componentID;
		component->horizontalSamplingFactor = samplingFactor >> 4;
		component->verticalSamplingFactor = samplingFactor & 0x0F;
		//if (component->horizontalSamplingFactor != 1 || component->verticalSamplingFactor != 1) {
		//	std::cout << "Error: sampling dimension factors unsupported for " << (uint)component->horizontalSamplingFactor << " and " << (uint)component->verticalSamplingFactor << "\n";
		//	jpeg->isValid = false;
		//	return;
		//}
		component->quantizationTableID = inFile.get();
		if (component->quantizationTableID > 3) {
			ErrorHandler::logJPEGError("Error: Component " + std::to_string((uint)componentID) + " is referencing an invalid QT ID: " + std::to_string((uint)component->quantizationTableID) + "\n", jpeg->isValid);
			return;
		}
	}
	if (length - 8 - (3 * jpeg->numComponents) != 0) {
		ErrorHandler::logJPEGError("Error: SOF Length Invalid\n", jpeg->isValid);
	}
}

void parseRI(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing DRI Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	if (length != 4) {
		ErrorHandler::logJPEGError("Error: Invalid DRI Length\n", jpeg->isValid);
	}
	jpeg->restartInterval = (inFile.get() << 8) | inFile.get();
}

void parseHT(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing DHT Marker\n";
	int length = (inFile.get() << 8) | inFile.get();
	length -= 2;
	while (length > 0) {
		byte tableInfo = inFile.get();
		byte tableID = tableInfo & 0x0F;
		bool ACTable = tableInfo >> 4;
		if (tableID > 3) {
			ErrorHandler::logJPEGError("Error: Invalid Huffman Table ID\n", jpeg->isValid);
			return;
		}
		HuffmanTable* hTable = ACTable ? &jpeg->huffmanACTables[tableID] : &jpeg->huffmanDCTables[tableID];
		hTable->set = true;
		hTable->offsets[0] = 0;
		uint allSymbols = 0;
		for (uint i = 1; i <= 16; ++i) {
			allSymbols += inFile.get();
			hTable->offsets[i] = allSymbols;
		}
		if (allSymbols > 162) {
			ErrorHandler::logJPEGError("Error: Too many symbols in Huffman Table\n", jpeg->isValid);
			return;
		}
		for (uint i = 0; i < allSymbols; ++i) {
			hTable->symbols[i] = inFile.get();
		}
		length -= 17 + allSymbols;
	}
	if (length != 0) {
		ErrorHandler::logJPEGError("Error: Invalid DHT", jpeg->isValid);
	}
}

void parseSOS(std::ifstream& inFile, JPEGImage* const jpeg) {
	std::cout << "Parsing SOS Marker\n";
	if (jpeg->numComponents == 0) {
		ErrorHandler::logJPEGError("Error: SOS Marker can't appear before SOF Marker\n", jpeg->isValid);
		return;
	}
	uint length = (inFile.get() << 8) | inFile.get();
	for (uint i = 0; i < jpeg->numComponents; ++i) {
		jpeg->colorComponents[i].used = false;
	}
	byte numOfComponents = inFile.get();
	for (uint i = 0; i < numOfComponents; ++i) {
		byte componentID = inFile.get();
		if (jpeg->zeroBased) {
			componentID += 1;
		}
		if (componentID > jpeg->numComponents) {
			ErrorHandler::logJPEGError("Error: Invalid color component ID " + std::to_string((uint)componentID) + "\n", jpeg->isValid);
			return;
		}
		ColorComponent* colorComponent = &jpeg->colorComponents[componentID - 1];
		if (colorComponent->used) {
			ErrorHandler::logJPEGError("Error: Dupelicate color component ID " + std::to_string((uint)componentID) + " not allowed\n", jpeg->isValid);
			return;
		}
		colorComponent->used = true;
		byte huffmanTableIDs = inFile.get();
		colorComponent->huffmanDCTableID = huffmanTableIDs >> 4;
		colorComponent->huffmanACTableID = huffmanTableIDs & 0x0F;
		if (colorComponent->huffmanACTableID > 3 || colorComponent->huffmanDCTableID > 3) {
			ErrorHandler::logJPEGError("Error: Component cannot refer to a huffman table of ID greater then 3\n", jpeg->isValid);
			return;
		}
	}

	// Spectral selection and successive approximation bytes:
	// in baseline: start = 0, end = 63, succcessiveApprox = 00 (0 for high and low byte)
	jpeg->startOfSelection = inFile.get();
	jpeg->endOfSelection = inFile.get();
	byte successiveApprox = inFile.get();
	jpeg->successiveApproximationHigh = successiveApprox >> 4;
	jpeg->successiveApproximationLow = successiveApprox & 0x0F;

	if (jpeg->frameType == baseline) {
		jpeg->startOfSelection = 0;
		jpeg->endOfSelection = 63;
		jpeg->successiveApproximationHigh = 0;
		jpeg->successiveApproximationLow = 0;
	}

	if (jpeg->startOfSelection != 0 || jpeg->endOfSelection != 63 || successiveApprox != 0) {
		ErrorHandler::logJPEGError("Error: Spectral selection, start: " +
			std::to_string((uint)jpeg->startOfSelection) + ", end: " +
			std::to_string((uint)jpeg->endOfSelection) + ", or successive approximation: " +
			std::to_string((uint)successiveApprox) + " are of illegal values\n",
			jpeg->isValid);
		return;
	}

	if (length - 6 - (numOfComponents * 2) != 0) {
		ErrorHandler::logJPEGError("Error: Invalid Length of SOS non-stream segment\n", jpeg->isValid);
		return;
	}
}

JPEGImage* parseJPEG(const std::string& filename) {
	std::ifstream inFile = std::ifstream(filename, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) {
		std::cout << "Error: Could not open file\n";
		return nullptr;
	}
	JPEGImage* jpeg = new (std::nothrow) JPEGImage;
	if (jpeg == nullptr) {
		std::cout << "Error: jpeg is null pointer\n";
		inFile.close();
		return nullptr;
	}
	byte last = inFile.get();
	byte current = inFile.get();
	if (last != 0xFF || current != SOI) {
		ErrorHandler::logJPEGError("Invalid Beginning of JPEG\n", jpeg->isValid, inFile);
		return jpeg;
	}
	last = inFile.get();
	current = inFile.get();
	while (jpeg->isValid) {
		if (!inFile) {
			ErrorHandler::logJPEGError("Error: Invalid end of JPEG\n", jpeg->isValid, inFile);
			return jpeg;
		}
		if (last != 0xFF) {
			ErrorHandler::logJPEGError("Error: Expected a marker\n", jpeg->isValid, inFile);
			return jpeg;
		}
		if (current >= APP0 && current <= APP15) { // APPN Discarding
			parseAPPN(inFile, jpeg);
		}
		else if (current == DQT) { // Define Quantization Table
			parseQT(inFile, jpeg);

		}
		else if (current == DRI){ // Define Restart Interval
			parseRI(inFile, jpeg);
		}
		else if (current == SOS) { // Start of Scan
			parseSOS(inFile, jpeg);
			break;
		}
		else if (current == DHT) { // Define Huffman Table
			parseHT(inFile, jpeg);
		}
		else if (current == SOF0) { // Start of Frame0
			jpeg->frameType = SOF0;
			parseSOF(inFile, jpeg);
		}
		else if (current == COM) { // Comment
			parseCOM(inFile, jpeg);
		}
		else if ((current >= JPG0 && current <= JPG13) || current == DNL || current == DHP || current == EXP) { // Ignore unused markers
			parseCOM(inFile, jpeg);
		}
		else if (current == 0xFF) { // Allows any number of consecutive 0xFF bytes
			current = inFile.get();
			continue;
		}
		else if (current == EOI) {
			ErrorHandler::logJPEGError("Error: EOI Marker before SOS is not allowed\n", jpeg->isValid, inFile);
			return jpeg;
		}
		else if (current == SOI) {
			ErrorHandler::logJPEGError("Error: Embedded JPEGs unsupported\n", jpeg->isValid, inFile);
			return jpeg;
		}
		else if (current == DAC) {
			ErrorHandler::logJPEGError("Error: Arithmetic mode unsupported\n", jpeg->isValid, inFile);
			return jpeg;
		}
		else if (current > SOF0 && current <= SOF15) {
			ErrorHandler::logJPEGError((std::ostringstream() << "Error: SOF1-15 unsupported, received: " << std::hex << (uint)current << std::dec << "\n").str(),
				jpeg->isValid, inFile);
			return jpeg;
		}
		else if (current >= RST0 && current <= RST7) {
			ErrorHandler::logJPEGError("Error: RST Marker before SOS is not allowed\n", jpeg->isValid, inFile);
			return jpeg;
		}
		else{
			ErrorHandler::logJPEGError((std::ostringstream() << "Error: unknown marker: " << std::hex << (uint)current << std::dec << "\n").str(),
				jpeg->isValid, inFile);
			return jpeg;
		}
		last = inFile.get();
		current = inFile.get();
	}
	if (jpeg->isValid) {
		current = inFile.get();
		while (true) {
			if (!inFile) {
				ErrorHandler::logJPEGError("Error: Bit-Stream prematurely ended\n", jpeg->isValid, inFile);
				return jpeg;
			}

			last = current;
			current = inFile.get();
			if (last == 0xFF) {
				if (current == EOI) {
					break;
				}
				else if (current == 0x00) { // overwrite 0x00 with the next byte
					jpeg->huffmanData.push_back(last);
					current = inFile.get();
				}
				else if (current >= RST0 && current <= RST7) { // overwrite 
					current = inFile.get();
				}
				else if (current == 0xFF) {
					// Do nothing
				}
			}
			else
			{
				jpeg->huffmanData.push_back(last);
			}
		}
	}
	return jpeg;
}

void printjpeg(const JPEGImage* const jpeg) {
	if (jpeg == nullptr) return;
	std::cout << "****DQT****\n";
	for (uint i = 0; i < 4; ++i) {
		if (jpeg->quantizationTables[i].set) {
			std::cout << "Table ID: " << i << "\n";
			std::cout << "\tTable Data:";
			for (uint k = 0; k < 64; ++k) {
				if (k % 8 == 0) std::cout << "\n\t\t";
				uint val = jpeg->quantizationTables[i].table[k];
				if(val < 10)
					std::cout << val << "   ";
				else if (val < 100)
					std::cout << val << "  ";
				else
					std::cout << val << " ";
			}
			std::cout << "\n\n";
		}
	}
	std::cout << "****SOF****\n";
	std::cout << "Frametype: 0x" << std::hex << (uint)jpeg->frameType << std::dec << "\n";
	std::cout << "Height: " << jpeg->height << "\n";
	std::cout << "Width: " << jpeg->width << "\n\n";
	std::cout << "Color Components:\n";
	for (int i = 0; i < jpeg->numComponents; ++i) {
		std::cout << "\tComponent ID: " << (uint)jpeg->colorComponents[i].componentID << "\n";
		std::cout << "\t\tHorizontal Sampling Factor: " << (uint)jpeg->colorComponents[i].horizontalSamplingFactor << "\n";
		std::cout << "\t\tVertical Sampling Factor: " << (uint)jpeg->colorComponents[i].verticalSamplingFactor << "\n";
		std::cout << "\t\tQuantization Table ID: " << (uint)jpeg->colorComponents[i].quantizationTableID << "\n\n";
	}
	std::cout << "***DHT***\n";
	std::cout << "DC Tables:\n";
	for (uint i = 0; i < 4; ++i) {
		if (jpeg->huffmanDCTables[i].set) {
			std::cout << "\tTable ID: " << i << "\n";
			std::cout << "\t\tSymbols:\n";
			for (uint j = 0; j < 16; ++j) {
				std::cout <<"\t\t" << (j + 1) << ": ";
				for (uint k = jpeg->huffmanDCTables[i].offsets[j]; k < jpeg->huffmanDCTables[i].offsets[j + 1]; ++k) {
					std::cout << std::hex << (uint)jpeg->huffmanDCTables[i].symbols[k] << std::dec << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
		}
	}
	std::cout << "\nAC Tables:\n";
	for (uint i = 0; i < 4; ++i) {
		if (jpeg->huffmanACTables[i].set) {
			std::cout << "\tTable ID: " << i << "\n";
			std::cout << "\t\tSymbols:\n";
			for (uint j = 0; j < 16; ++j) {
				std::cout <<"\t\t" << (j + 1) << ": ";
				for (uint k = jpeg->huffmanACTables[i].offsets[j]; k < jpeg->huffmanACTables[i].offsets[j + 1]; ++k) {
					std::cout << std::hex << (uint)jpeg->huffmanACTables[i].symbols[k] << std::dec << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
		}
	}
	std::cout << "\n***DRI***\n";
	std::cout << "Restart Interval: " << (uint)jpeg->restartInterval << "\n\n";
	std::cout << "\n***SOS***\n";
	std::cout << "Components:\n";
	for (uint i = 0; i < jpeg->numComponents; ++i) {
		std::cout << "\tComponent ID: " << (uint)jpeg->colorComponents[i].componentID << "\n";
		std::cout << "\tHuffman DC Table ID: " << (uint)jpeg->colorComponents[i].huffmanDCTableID << "\n";
		std::cout << "\tHuffman AC Table ID: " << (uint)jpeg->colorComponents[i].huffmanACTableID << "\n\n";
	}
	std::cout << "Spectral selection:\n";
	std::cout << "\tStart of Selection: " << (uint)jpeg->startOfSelection << "\n";
	std::cout << "\tEnd of Selection: " << (uint)jpeg->endOfSelection << "\n";
	std::cout << "Successive Approximation:\n";
	std::cout << "\tHigh: " << (uint)jpeg->successiveApproximationHigh << "\n";
	std::cout << "\tLow: " << (uint)jpeg->successiveApproximationLow << "\n";
	std::cout << "\tLength of Huffman Data: " << (uint)jpeg->huffmanData.size() << "\n";
}
