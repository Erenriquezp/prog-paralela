#ifndef FILTRO_H
#define FILTRO_H

#include <cstdint>

void filtro_escala_grises_simd(const uint8_t* rgba_pixels, uint8_t* gray_pixels, int width, int height);
void filtro_escala_grises_regiones(const uint8_t* rgba_pixels, uint8_t* gray_pixels, int width, int height);

#endif