#ifndef PRODUCTO_ESCALAR_H
#define PRODUCTO_ESCALAR_H

#include <cstdint>
float producto_escalar_simd(const float* X, const float* Y, uint32_t n);
float producto_escalar_regiones(const float* X, const float* Y, uint32_t n);
#endif