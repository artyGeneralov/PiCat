#include "include/jpeg.h"
#include <iostream>

struct JPEGImage;
JPEGImage* parseJPEG(const std::string&);
void printjpeg(const JPEGImage* const);
MCU* decodeHuffmanData(JPEGImage* const);
void writeBMP(const std::string&, const MCU* const, const JPEGImage*);
void dequantize(const JPEGImage* const, MCU* const);
void inverseDCT(const JPEGImage* const, MCU* const);
void convertToRGB(const JPEGImage*, MCU* const);

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
		MCU* mcus = decodeHuffmanData(jpeg);
		if (mcus == nullptr) {
			std::cout << "MCU Array Deleted\n";
			delete jpeg;
			continue;
		}

		dequantize(jpeg, mcus);

		inverseDCT(jpeg, mcus);

		convertToRGB(jpeg, mcus);

		const std::size_t pos = filename.find_last_of(".");
		const std::string outName = (pos == std::string::npos) ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");
		writeBMP(outName, mcus, jpeg);


		delete[] mcus;
		delete jpeg;
	}
}
