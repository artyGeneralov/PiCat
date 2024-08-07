#include "include/jpeg.h"
#include <iostream>

struct JPEGImage;
JPEGImage* parseJPEG(const std::string&);
void printjpeg(const JPEGImage* const);

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Error: No file specified for conversion\n";
		return 0;
	}

	for (int i = 1; i < argc; ++i) {
		const std::string filename(argv[i]);

		// read jpeg
		JPEGImage* jpeg = parseJPEG(filename);

		// validate jpeg
		if (jpeg == nullptr) {
			continue;
		}
		else if (jpeg->isValid == false)
		{
			std::cout << "Error: Provided file " + filename + " is an invalid JPEG\n";
			delete jpeg;
			continue;
		}

		printjpeg(jpeg);

		// decode Huffman data


		delete jpeg;
	}
}
