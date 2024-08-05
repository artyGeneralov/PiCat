#include "jpeg.h"
#include <iostream>
#include <fstream>

void parseQT(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing DQT Marker\n";
	int length = (inFile.get() << 8) | inFile.get();
	length -= 2;
	while (length > 0) {
		byte tableInfo = inFile.get(); // lower nibble holds table id, upper nibble holds the amount of bits
		length -= 1;
		byte tableID = tableInfo & 0x0F;
		if (tableID > 3) {
			std::cout << "Error: Invalid Quatization Table ID " << (uint)tableID << "\n";
			header->isValid = false;
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
		std::cout << "Error: DQT Marker invalid size\n";
	}
}

void parseAPPN(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing APPN Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	for (int i = 0; i < length - 2; ++i) {
		inFile.get();
	}
}

void parseSOF(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing SOF Marker\n";
	if (header->numComponents != 0) {
		std::cout << "Error: Duplicate SOF Markers\n";
		header->isValid = false;
		return;
	}
	uint length = (inFile.get() << 8) | inFile.get();
	byte precision = inFile.get();
	if (precision != 8) {
		std::cout << "Error: Invalid precision. Must be 8, received " << (uint)precision << "\n";
		header->isValid = false;
		return;
	}

	header->height = (inFile.get() << 8) | inFile.get();
	header->width = (inFile.get() << 8) | inFile.get();
	if (header->height == 0 || header->width == 0)
	{
		std::cout << "Error: Invalid dimensions\n";
		header->isValid = false;
		return;
	}
	header->numComponents = inFile.get();
	if (header->numComponents != 3 && header->numComponents != 1) {
		std::cout << "Error: Invalid number of components";
		header->isValid = false;
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
			std::cout << "Error: YIQ Color mode not supported\n"; // TODO: Support YIQ Color mode
			header->isValid = false;
			return;
		}
		if (componentID == 0 || componentID > 3) {
			std::cout << "Error: Invalid Component IDs\n";
			header->isValid = false;
			return;
		}
		ColorComponent* component = &header->colorComponents[componentID - 1];
		if (component->used) {
			std::cout << "Error: Duplicate component definition for component: " << (uint)componentID << "\n";
			header->isValid = false;
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
			std::cout << "Error: Component " << (uint)componentID << " is referencing an invalid QT ID: " << (uint)component->quantizationTableID << "\n";
			header->isValid = false;
			return;
		}
	}
	if (length - 8 - (3 * header->numComponents) != 0) {
		std::cout << "Error: SOF Length Invalid";
		header->isValid = false;
	}
}

void parseRI(std::ifstream& inFile, Header* const header) {
	std::cout << "Parsing DRI Marker\n";
	uint length = (inFile.get() << 8) | inFile.get();
	if (length != 4) {
		std::cout << "Error: Invalid DRI Length\n";
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
			std::cout << "Error: Invalid Huffman Table ID\n";
			header->isValid = false;
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
			std::cout << "Error: Too many symbols in Huffman Table\n";
			header->isValid = false;
			return;
		}
		for (uint i = 0; i < allSymbols; ++i) {
			hTable->symbols[i] = inFile.get();
		}
		length -= 17 + allSymbols;
	}
	if (length != 0) {
		std::cout << "Error: Invalid DHT";
		header->isValid = false;
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
		header->isValid = false;
		inFile.close();
		return header;
	}
	last = inFile.get();
	current = inFile.get();
	int count = 0; // delete
	while (header->isValid) {
		if (!inFile) {
			std::cout << "Error: Invalid end of file\n";
			header->isValid = false;
			inFile.close();
			return header;
		}
		if (last != 0xFF) {
			std::cout << "Error: Expected a marker\n";
			header->isValid = false;
			inFile.close();
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
		else if (current == DHT) { // Define Huffman Table
			parseHT(inFile, header);
			count++; //delete
			if (count > 3) break; //delete
		}
		else if (current == SOF0) { // Start of Frame0
			header->frameType = SOF0;
			parseSOF(inFile, header);
		}
		last = inFile.get();
		current = inFile.get();
	}
	if(header->isValid)
		std::cout << "File read correctly\n";
	return header;
}

void printHeader(const Header* const header) {
	if (header == nullptr) return;
	std::cout << "****DQT****\n";
	for (uint i = 0; i < 4; ++i) {
		if (header->quantizationTables[i].set) {
			std::cout << "Table ID: " << i << "\n";
			std::cout << "Table Data:\n";
			for (uint k = 0; k < 64; ++k) {
				if (k % 8 == 0) std::cout << "\n";
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
	std::cout << "Color Components:\n\n";
	for (int i = 0; i < header->numComponents; ++i) {
		std::cout << "Component ID: " << (uint)header->colorComponents[i].componentID << "\n";
		std::cout << "Horizontal Sampling Factor: " << (uint)header->colorComponents[i].horizontalSamplingFactor << "\n";
		std::cout << "Vertical Sampling Factor: " << (uint)header->colorComponents[i].verticalSamplingFactor << "\n";
		std::cout << "Quantization Table ID: " << (uint)header->colorComponents[i].quantizationTableID << "\n\n";
	}
	std::cout << "***DHT***\n";
	std::cout << "DC Tables:\n";
	for (uint i = 0; i < 4; ++i) {
		if (header->huffmanDCTables[i].set) {
			std::cout << "Table ID: " << i << "\n";
			std::cout << "Symbols:\n";
			for (uint j = 0; j < 16; ++j) {
				std::cout << (j + 1) << ": ";
				for (uint k = header->huffmanDCTables[i].offsets[j]; k < header->huffmanDCTables[i].offsets[j + 1]; ++k) {
					std::cout << std::hex << (uint)header->huffmanDCTables[i].symbols[k] << std::dec << " ";
				}
				std::cout << "\n";
			}
		}
	}
	std::cout << "\nAC Tables:\n";
	for (uint i = 0; i < 4; ++i) {
		if (header->huffmanACTables[i].set) {
			std::cout << "Table ID: " << i << "\n";
			std::cout << "Symbols:\n";
			for (uint j = 0; j < 16; ++j) {
				std::cout << (j + 1) << ": ";
				for (uint k = header->huffmanACTables[i].offsets[j]; k < header->huffmanACTables[i].offsets[j + 1]; ++k) {
					std::cout << std::hex << (uint)header->huffmanACTables[i].symbols[k] << std::dec << " ";
				}
				std::cout << "\n";
			}
		}
	}
	std::cout << "\n***DRI***\n";
	std::cout << "Restart Interval: " << (uint)header->restartInterval << "\n\n";
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Error: No file specified for conversion\n";
		return 0;
	}

	for (int i = 1; i < argc; ++i) {
		const std::string filename(argv[i]);

		// read header
		Header* header = parseJPEG(filename);

		// validate header
		if (header == nullptr) {
			continue;
		}
		else if (header->isValid == false)
		{
			std::cout << "Error: Provided file " + filename + " is an invalid JPEG\n";
			delete header;
			continue;
		}

		printHeader(header);

		// decode Huffman data


		delete header;
	}
}