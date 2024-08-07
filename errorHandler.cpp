#include "errorHandler.h"

void logJPEGError(const std::string& message, bool& isValid) {
	std::cout << message;
	isValid = false;
}

void logJPEGError(const std::string& message, bool& isValid, std::ifstream& inFile) {
	std::cout << message;
	isValid = false;
	inFile.close();
}