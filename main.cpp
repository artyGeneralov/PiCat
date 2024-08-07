#include "include/jpeg.h"
#include <iostream>

struct Header;
Header* parseJPEG(const std::string&);
void printHeader(const Header* const);

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
