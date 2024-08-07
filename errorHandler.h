#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

class ErrorHandler {
public:
	static void logJPEGError(const std::string& msg, bool& isValid);
	static void logJPEGError(const std::string& msg, bool& isValid, std::ifstream& inFile);
};


#endif // ERROR_HANDLER_H
