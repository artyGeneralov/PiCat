#include "../include/errorHandler.h"

void ErrorHandler::logJPEGError(const std::string& message, bool& isValid) {
	std::cout << message;
	isValid = false;
}

void ErrorHandler::logJPEGError(const std::string& message, bool& isValid, std::ifstream& inFile) {
	std::cout << message;
	isValid = false;
	inFile.close();
}