#ifndef FRACTAL_SIMD_H
#define FRACTAL_SIMD_H
#include <cstdint>

void julia_simd(double x_min, double y_min, double x_max, double y_max, uint32_t width, uint32_t height, uint32_t* pixel_buffer);

#endif // FRACTAL_SIMD_H