#include "../include/jpeg.h"
#include <iostream>
#include <fstream>
#include "../include/errorHandler.h"


void parseQT(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing DQT Marker\n";
	int length = (inFile.get() << 8) | inFile.get();
	length -= 2;
	while (length > 0) {
		byte tableInfo = inFile.get(); // lower nibble holds table id, upper nibble holds the amount of bits
		length -= 1;
		byte tableID = tableInfo & 0x0F;
		if (tableID > 3) {
			ErrorHandler::logJPEGError("Error: Invalid Quatization Table ID " + std::to_string((uint)tableID) + "\n", header->isValid);
			return;
		}
		header->quantizationTables[tableID].set = true;
		if (tableInfo >> 4 != 0) { // 16-bit quantization table
			for (uint i = 0; i < 64; ++i) {
				header->quantizationTables[tableID].table[zigZagMap[i]] = (inFile.get() << 8) | inFile.get();
			}
			length -= 128;
		}
		else { // 8-bit quantization table
			for (uint i = 0; i < 64; ++i) {
				header->quantizationTables[tableID].table[zigZagMap[i]] = inFile.get();
			}
			length -= 64;
		}
	}
	if (length != 0) {
		ErrorHandler::logJPEGError("Error: DQT Marker invalid size\n", header->isValid);
	}
}

void parseAPPN(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing APPN Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	for (uint i = 0; i < length - 2; ++i) {
		inFile.get();
	}
}

void parseCOM(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing COM Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	for (uint i = 0; i < length - 2; ++i) {
		inFile.get();
	}
}

void parseSOF(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing SOF Marker\n";
	if (header->numComponents != 0) {
		ErrorHandler::logJPEGError("Error: Duplicate SOF Markers\n", header->isValid);
		return;
	}
	uint length = (inFile.get() << 8) | inFile.get();
	byte precision = inFile.get();
	if (precision != 8) {
		ErrorHandler::logJPEGError("Error: Invalid precision. Must be 8, received " + std::to_string((uint)precision) + "\n", header->isValid);
		return;
	}

	header->height = (inFile.get() << 8) | inFile.get();
	header->width = (inFile.get() << 8) | inFile.get();
	if (header->height == 0 || header->width == 0){
		ErrorHandler::logJPEGError("Error: Invalid dimensions\n", header->isValid);
		return;
	}
	header->numComponents = inFile.get();
	if (header->numComponents != 3 && header->numComponents != 1) {
		ErrorHandler::logJPEGError("Error: Invalid number of components", header->isValid);
		return;
	}
	for (uint i = 0; i < header->numComponents; ++i) {
		byte componentID = inFile.get();
		// Component ID can be 1, 2 or 3 in YCrCb Color mode
		// In rare cases ID can be 0, 1 or 2, So we force it to the range 1, 2 and 3
		if (componentID == 0) {
			header->zeroBased = true;
		}
		if (header->zeroBased == true) {
			componentID += 1;
		}
		if (componentID == 4 || componentID == 5) {
			ErrorHandler::logJPEGError("Error: YIQ Color mode not supported\n", header->isValid); // TODO: Support YIQ Color mode
			return;
		}
		if (componentID == 0 || componentID > 3) {
			ErrorHandler::logJPEGError("Error: Invalid Component IDs\n", header->isValid);
			return;
		}
		ColorComponent* component = &header->colorComponents[componentID - 1];
		if (component->used) {
			ErrorHandler::logJPEGError("Error: Duplicate component definition for component: " + std::to_string((uint)componentID) + "\n", header->isValid);
			return;
		}
		component->used = true;
		byte samplingFactor = inFile.get();
		component->componentID = componentID;
		component->horizontalSamplingFactor = samplingFactor >> 4;
		component->verticalSamplingFactor = samplingFactor & 0x0F;
		//if (component->horizontalSamplingFactor != 1 || component->verticalSamplingFactor != 1) {
		//	std::cout << "Error: sampling dimension factors unsupported for " << (uint)component->horizontalSamplingFactor << " and " << (uint)component->verticalSamplingFactor << "\n";
		//	header->isValid = false;
		//	return;
		//}
		component->quantizationTableID = inFile.get();
		if (component->quantizationTableID > 3) {
			ErrorHandler::logJPEGError("Error: Component " + std::to_string((uint)componentID) + " is referencing an invalid QT ID: " + std::to_string((uint)component->quantizationTableID) + "\n", header->isValid);
			return;
		}
	}
	if (length - 8 - (3 * header->numComponents) != 0) {
		ErrorHandler::logJPEGError("Error: SOF Length Invalid\n", header->isValid);
	}
}

void parseRI(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing DRI Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	if (length != 4) {
		ErrorHandler::logJPEGError("Error: Invalid DRI Length\n", header->isValid);
	}
	header->restartInterval = (inFile.get() << 8) | inFile.get();
}

void parseHT(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing DHT Marker\n";
	int length = (inFile.get() << 8) | inFile.get();
	length -= 2;
	while (length > 0) {
		byte tableInfo = inFile.get();
		byte tableID = tableInfo & 0x0F;
		bool ACTable = tableInfo >> 4;
		if (tableID > 3) {
			ErrorHandler::logJPEGError("Error: Invalid Huffman Table ID\n", header->isValid);
			return;
		}
		HuffmanTable* hTable = ACTable ? &header->huffmanACTables[tableID] : &header->huffmanDCTables[tableID];
		hTable->set = true;
		hTable->offsets[0] = 0;
		uint allSymbols = 0;
		for (uint i = 1; i <= 16; ++i) {
			allSymbols += inFile.get();
			hTable->offsets[i] = allSymbols;
		}
		if (allSymbols > 162) {
			ErrorHandler::logJPEGError("Error: Too many symbols in Huffman Table\n", header->isValid);
			return;
		}
		for (uint i = 0; i < allSymbols; ++i) {
			hTable->symbols[i] = inFile.get();
		}
		length -= 17 + allSymbols;
	}
	if (length != 0) {
		ErrorHandler::logJPEGError("Error: Invalid DHT", header->isValid);
	}
}

void parseSOS(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing SOS Marker\n";
	if (header->numComponents == 0) {
		ErrorHandler::logJPEGError("Error: SOS Marker can't appear before SOF Marker\n", header->isValid);
		return;
	}
	uint length = (inFile.get() << 8) | inFile.get();
	for (uint i = 0; i < header->numComponents; ++i) {
		header->colorComponents[i].used = false;
	}
	byte numOfComponents = inFile.get();
	for (uint i = 0; i < numOfComponents; ++i) {
		byte componentID = inFile.get();
		if (header->zeroBased) {
			componentID += 1;
		}
		if (componentID > header->numComponents) {
			ErrorHandler::logJPEGError("Error: Invalid color component ID " + std::to_string((uint)componentID) + "\n", header->isValid);
			return;
		}
		ColorComponent* colorComponent = &header->colorComponents[componentID - 1];
		if (colorComponent->used) {
			ErrorHandler::logJPEGError("Error: Dupelicate color component ID " + std::to_string((uint)componentID) + " not allowed\n", header->isValid);
			return;
		}
		colorComponent->used = true;
		byte huffmanTableIDs = inFile.get();
		colorComponent->huffmanDCTableID = huffmanTableIDs >> 4;
		colorComponent->huffmanACTableID = huffmanTableIDs & 0x0F;
		if (colorComponent->huffmanACTableID > 3 || colorComponent->huffmanDCTableID > 3) {
			ErrorHandler::logJPEGError("Error: Component cannot refer to a huffman table of ID greater then 3\n", header->isValid);
			return;
		}
	}

	// Spectral selection and successive approximation bytes:
	// in baseline: start = 0, end = 63, succcessiveApprox = 00 (0 for high and low byte)
	header->startOfSelection = inFile.get();
	header->endOfSelection = inFile.get();
	byte successiveApprox = inFile.get();
	header->successiveApproximationHigh = successiveApprox >> 4;
	header->successiveApproximationLow = successiveApprox & 0x0F;

	if (header->frameType == baseline) {
		header->startOfSelection = 0;
		header->endOfSelection = 63;
		header->successiveApproximationHigh = 0;
		header->successiveApproximationLow = 0;
	}

	if (header->startOfSelection != 0 || header->endOfSelection != 63 || successiveApprox != 0) {
		ErrorHandler::logJPEGError("Error: Spectral selection, start: " +
			std::to_string((uint)header->startOfSelection) + ", end: " +
			std::to_string((uint)header->endOfSelection) + ", or successive approximation: " +
			std::to_string((uint)successiveApprox) + " are of illegal values\n",
			header->isValid);
		return;
	}

	if (length - 6 - (numOfComponents * 2) != 0) {
		ErrorHandler::logJPEGError("Error: Invalid Length of SOS non-stream segment\n", header->isValid);
		return;
	}
}

Header* parseJPEG(const std::string& filename) {
	std::ifstream inFile = std::ifstream(filename, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) {
		std::cout << "Error: Could not open file\n";
		return nullptr;
	}
	Header* header = new (std::nothrow) Header;
	if (header == nullptr) {
		std::cout << "Error: header is null pointer\n";
		inFile.close();
		return nullptr;
	}
	byte last = inFile.get();
	byte current = inFile.get();
	if (last != 0xFF || current != SOI) {
		ErrorHandler::logJPEGError("Invalid Beginning of JPEG\n", header->isValid, inFile);
		return header;
	}
	last = inFile.get();
	current = inFile.get();
	while (header->isValid) {
		if (!inFile) {
			ErrorHandler::logJPEGError("Error: Invalid end of JPEG\n", header->isValid, inFile);
			return header;
		}
		if (last != 0xFF) {
			ErrorHandler::logJPEGError("Error: Expected a marker\n", header->isValid, inFile);
			return header;
		}
		if (current >= APP0 && current <= APP15) { // APPN Discarding
			parseAPPN(inFile, header);
		}
		else if (current == DQT) { // Define Quantization Table
			parseQT(inFile, header);

		}
		else if (current == DRI){ // Define Restart Interval
			parseRI(inFile, header);
		}
		else if (current == SOS) { // Start of Scan
			parseSOS(inFile, header);
			break;
		}
		else if (current == DHT) { // Define Huffman Table
			parseHT(inFile, header);
		}
		else if (current == SOF0) { // Start of Frame0
			header->frameType = SOF0;
			parseSOF(inFile, header);
		}
		else if (current == COM) { // Comment
			parseCOM(inFile, header);
		}
		else if ((current >= JPG0 && current <= JPG13) || current == DNL || current == DHP || current == EXP) { // Ignore unused markers
			parseCOM(inFile, header);
		}
		else if (current == 0xFF) { // Allows any number of consecutive 0xFF bytes
			current = inFile.get();
			continue;
		}
		else if (current == EOI) {
			ErrorHandler::logJPEGError("Error: EOI Marker before SOS is not allowed\n", header->isValid, inFile);
			return header;
		}
		else if (current == SOI) {
			ErrorHandler::logJPEGError("Error: Embedded JPEGs unsupported\n", header->isValid, inFile);
			return header;
		}
		else if (current == DAC) {
			ErrorHandler::logJPEGError("Error: Arithmetic mode unsupported\n", header->isValid, inFile);
			return header;
		}
		else if (current > SOF0 && current <= SOF15) {
			ErrorHandler::logJPEGError((std::ostringstream() << "Error: SOF1-15 unsupported, received: " << std::hex << (uint)current << std::dec << "\n").str(),
				header->isValid, inFile);
			return header;
		}
		else if (current >= RST0 && current <= RST7) {
			ErrorHandler::logJPEGError("Error: RST Marker before SOS is not allowed\n", header->isValid, inFile);
			return header;
		}
		else{
			ErrorHandler::logJPEGError((std::ostringstream() << "Error: unknown marker: " << std::hex << (uint)current << std::dec << "\n").str(),
				header->isValid, inFile);
			return header;
		}
		last = inFile.get();
		current = inFile.get();
	}
	if (header->isValid) {
		current = inFile.get();
		while (true) {
			if (!inFile) {
				ErrorHandler::logJPEGError("Error: Bit-Stream prematurely ended\n", header->isValid, inFile);
				return header;
			}

			last = current;
			current = inFile.get();
			if (last == 0xFF) {
				if (current == EOI) {
					break;
				}
				else if (current == 0x00) { // overwrite 0x00 with the next byte
					header->huffmanData.push_back(last);
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
				header->huffmanData.push_back(last);
			}
		}
	}
	return header;
}

void printHeader(const Header* const header) {
	if (header == nullptr) return;
	std::cout << "****DQT****\n";
	for (uint i = 0; i < 4; ++i) {
		if (header->quantizationTables[i].set) {
			std::cout << "Table ID: " << i << "\n";
			std::cout << "\tTable Data:";
			for (uint k = 0; k < 64; ++k) {
				if (k % 8 == 0) std::cout << "\n\t\t";
				uint val = header->quantizationTables[i].table[k];
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
	std::cout << "Frametype: 0x" << std::hex << (uint)header->frameType << std::dec << "\n";
	std::cout << "Height: " << header->height << "\n";
	std::cout << "Width: " << header->width << "\n\n";
	std::cout << "Color Components:\n";
	for (int i = 0; i < header->numComponents; ++i) {
		std::cout << "\tComponent ID: " << (uint)header->colorComponents[i].componentID << "\n";
		std::cout << "\t\tHorizontal Sampling Factor: " << (uint)header->colorComponents[i].horizontalSamplingFactor << "\n";
		std::cout << "\t\tVertical Sampling Factor: " << (uint)header->colorComponents[i].verticalSamplingFactor << "\n";
		std::cout << "\t\tQuantization Table ID: " << (uint)header->colorComponents[i].quantizationTableID << "\n\n";
	}
	std::cout << "***DHT***\n";
	std::cout << "DC Tables:\n";
	for (uint i = 0; i < 4; ++i) {
		if (header->huffmanDCTables[i].set) {
			std::cout << "\tTable ID: " << i << "\n";
			std::cout << "\t\tSymbols:\n";
			for (uint j = 0; j < 16; ++j) {
				std::cout <<"\t\t" << (j + 1) << ": ";
				for (uint k = header->huffmanDCTables[i].offsets[j]; k < header->huffmanDCTables[i].offsets[j + 1]; ++k) {
					std::cout << std::hex << (uint)header->huffmanDCTables[i].symbols[k] << std::dec << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
		}
	}
	std::cout << "\nAC Tables:\n";
	for (uint i = 0; i < 4; ++i) {
		if (header->huffmanACTables[i].set) {
			std::cout << "\tTable ID: " << i << "\n";
			std::cout << "\t\tSymbols:\n";
			for (uint j = 0; j < 16; ++j) {
				std::cout <<"\t\t" << (j + 1) << ": ";
				for (uint k = header->huffmanACTables[i].offsets[j]; k < header->huffmanACTables[i].offsets[j + 1]; ++k) {
					std::cout << std::hex << (uint)header->huffmanACTables[i].symbols[k] << std::dec << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
		}
	}
	std::cout << "\n***DRI***\n";
	std::cout << "Restart Interval: " << (uint)header->restartInterval << "\n\n";
	std::cout << "\n***SOS***\n";
	std::cout << "Components:\n";
	for (uint i = 0; i < header->numComponents; ++i) {
		std::cout << "\tComponent ID: " << (uint)header->colorComponents[i].componentID << "\n";
		std::cout << "\tHuffman DC Table ID: " << (uint)header->colorComponents[i].huffmanDCTableID << "\n";
		std::cout << "\tHuffman AC Table ID: " << (uint)header->colorComponents[i].huffmanACTableID << "\n\n";
	}
	std::cout << "Spectral selection:\n";
	std::cout << "\tStart of Selection: " << (uint)header->startOfSelection << "\n";
	std::cout << "\tEnd of Selection: " << (uint)header->endOfSelection << "\n";
	std::cout << "Successive Approximation:\n";
	std::cout << "\tHigh: " << (uint)header->successiveApproximationHigh << "\n";
	std::cout << "\tLow: " << (uint)header->successiveApproximationLow << "\n";
	std::cout << "\tLength of Huffman Data: " << (uint)header->huffmanData.size() << "\n";
}
