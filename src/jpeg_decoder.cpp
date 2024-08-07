#include "../include/jpeg.h"

MCU* decodeJPEG_stub() {
	// Create a 24x24 RGB Image
	MCU* decoded_mcus = new MCU[9];
	
	//Set top 3 MCUs to red:
	for (uint i = 0; i < 3; ++i) {
		for (uint j = 0; j < 64; ++j) {
			decoded_mcus[i].r[j] = 255;
			decoded_mcus[i].g[j] = 0;
			decoded_mcus[i].b[j] = 0;
		}
	}
	//Set middle 3 MCUs to green:
	for (uint i = 0; i < 3; ++i) {
		for (uint j = 0; j < 64; ++j) {
			decoded_mcus[i].r[j] = 0;
			decoded_mcus[i].g[j] = 255;
			decoded_mcus[i].b[j] = 0;
		}
	}

	//Set bottom 3 MCUs to blue:
	for (uint i = 0; i < 3; ++i) {
		for (uint j = 0; j < 64; ++j) {
			decoded_mcus[i].r[j] = 0;
			decoded_mcus[i].g[j] = 0;
			decoded_mcus[i].b[j] = 255;
		}
	}

	return decoded_mcus;
}