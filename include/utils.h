#ifndef UTILS_H
#define UTILS_H

#include <fstream>
typedef unsigned char byte;
typedef unsigned int uint;
const double U_PI = std::acos(-1);

void putLong(std::ofstream&, const uint);
void putShort(std::ofstream&, const uint);


#endif // UTILS_H
