#ifndef TEST_H
#define TEST_H
#include <cstdint>

void simd(const uint8_t* entrada, uint8_t* salida, int w, int h);
void regiones(const uint8_t* entrada, uint8_t* salida, int w, int h);

#endif