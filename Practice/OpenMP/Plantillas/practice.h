#ifndef PRACTICE_H
#define PRACTICE_H
#include <cstdint>

void simdP(const uint8_t* rgba, uint8_t* sepia, int w, int h);
void regionsP(const uint8_t* rgba, uint8_t* sepia, int w, int h);

#endif