#include "fractal.h"
#include "palette.h"

#include <cmath>
#include <cstdint>

extern int max_iteraciones;

static uint32_t burning_ship_color(double cx, double cy)
{
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < max_iteraciones && (zr * zr + zi * zi) <= 4.0) {
        double ar = std::abs(zr);
        double ai = std::abs(zi);

        double next_zr = ar * ar - ai * ai + cx;
        double next_zi = 2.0 * ar * ai + cy;

        zr = next_zr;
        zi = next_zi;
        ++iter;
    }

    if (iter >= max_iteraciones) {
        return 0xFF000000;
    }

    return color_ramp[iter % PALETTE_SIZE];
}

void burning_mpi(double x_min, double y_min, double x_max, double y_max,
                 uint32_t width, uint32_t height,
                 uint32_t row_start, uint32_t row_end,
                 uint32_t* pixel_buffer)
{
    double dx = width > 1 ? (x_max - x_min) / (width - 1.0) : 0.0;
    double dy = height > 1 ? (y_max - y_min) / (height - 1.0) : 0.0;

    row_end = std::min(row_end, height);

    for (uint32_t row = row_start; row < row_end; ++row) {
        double cy = y_max - row * dy;
        for (uint32_t col = 0; col < width; ++col) {
            double cx = x_min + col * dx;
            pixel_buffer[(row - row_start) * width + col] = burning_ship_color(cx, cy);
        }
    }
}