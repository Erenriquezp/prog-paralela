#ifndef FRACTAL_SERIAL_H
#define FRACTAL_SERIAL_H

#include <cstdint>

void julia_serial_1(double x_min, double y_min, double x_max, double y_max, uint32_t width, uint32_t height, uint32_t* pixel_buffer);
void julia_serial_2(double x_min, double y_min, double x_max, double y_max, uint32_t width, uint32_t height, uint32_t* pixel_buffer);

#endif