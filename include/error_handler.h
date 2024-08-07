#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

class ErrorHandler {
public:
	static void logJPEGError(const std::string&, bool&);
	static void logJPEGError(const std::string&, bool&, std::ifstream&);
};


#endif // ERROR_HANDLER_H
