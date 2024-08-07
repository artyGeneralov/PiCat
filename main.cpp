#include "include/jpeg.h"
#include <iostream>

struct JPEGImage;
JPEGImage* parseJPEG(const std::string&);
void printjpeg(const JPEGImage* const);
MCU* decodeJPEG_stub();
void writeBMP(const std::string&, MCU*);

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
		MCU* mcus = decodeJPEG_stub();
		const std::size_t pos = filename.find_last_of(".");
		const std::string outName = (pos == std::string::npos ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");
		writeBMP(outName, mcus);


		delete[] mcus;
		delete jpeg;
	}
}
