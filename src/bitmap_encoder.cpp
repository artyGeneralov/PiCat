#include "../include/jpeg.h"
#include <iostream>
#include <fstream>

void writeBMP(const std::string& savefile_name, const MCU* const mcus, const JPEGImage* jpeg_data) {
	std::ofstream outFile = std::ofstream(savefile_name, std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		std::cout << "Error: Could not open output file\n";
		return;
	}

	const uint mcuRows = (jpeg_data->height + 7) / 8;
	const uint mcuCols = (jpeg_data->width + 7) / 8;
	const uint padding = jpeg_data->width % 4;
	const uint bmp_filesize = 14 + 12 + jpeg_data->height * jpeg_data->width * 3 + padding * jpeg_data->height;


	// Bitmap Header Structure:
	outFile.put('B');
	outFile.put('M');
	putLong(outFile, bmp_filesize);
	putLong(outFile, 0);
	putLong(outFile, 0x1A);
	// DIB Header:
	putLong(outFile, 12);
	putShort(outFile, jpeg_data->width);
	putShort(outFile, jpeg_data->height);
	putShort(outFile, 1);
	putShort(outFile, 24);

	#pragma warning(push)
	#pragma warning(disable: 6293)
	for (uint i = jpeg_data->height - 1; i < jpeg_data->height; --i) {
		const uint mcuRow = i / 8;
		const uint pixelRow = i % 8;
		for (uint k = 0; k < jpeg_data->width; ++k) {
			const uint mcuCol = k / 8;
			const uint pixelCol = k % 8;
			const uint mcuID = mcuCol + mcuRow * mcuCols;
			const uint pixelID = pixelRow * 8 + pixelCol;
			outFile.put(mcus[mcuID].b[pixelID]);
			outFile.put(mcus[mcuID].g[pixelID]);
			outFile.put(mcus[mcuID].r[pixelID]);
		}
		for (uint p = 0; p < padding; ++p) {
			outFile.put(0);
		}
	}
	outFile.close();
}