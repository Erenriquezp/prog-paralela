#include "fractal.h"
#include "palette.h"
#include <cmath>
#include <algorithm>

extern int max_iteraciones;

static const double EPS = 1e-4;
static const double roots_r[3] = { 1.0, -0.5, -0.5 };
static const double roots_i[3] = { 0.0, 0.8660254037844386, -0.8660254037844386 };

static uint32_t newton_color(double cx, double cy, int& out_iter)
{
    double zr = cx;
    double zi = cy;
    int iter = 0;

    while (iter < max_iteraciones) {
        double r2 = zr * zr + zi * zi;
        if (r2 > 4.0 || r2 < 1e-12) {
            out_iter = iter;
            return 0xFF000000;
        }

        double zr2 = zr * zr;
        double zi2 = zi * zi;
        double zr3 = zr * zr2 - 3.0 * zr * zi2;
        double zi3 = 3.0 * zr2 * zi - zi * zi2;

        double dr = 3.0 * (zr2 - zi2);
        double di = 6.0 * zr * zi;

        double denom = dr * dr + di * di;
        if (denom < 1e-12) {
            out_iter = iter;
            return 0xFF000000;
        }

        double nr = zr3 - 1.0;
        double ni = zi3;

        double fr = (nr * dr + ni * di) / denom;
        double fi = (ni * dr - nr * di) / denom;

        zr = zr - fr;
        zi = zi - fi;

        for (int k = 0; k < 3; ++k) {
            double dr2 = zr - roots_r[k];
            double di2 = zi - roots_i[k];
            if (dr2 * dr2 + di2 * di2 < EPS * EPS) {
                out_iter = iter;
                return color_ramp[(k * 5 + iter) % PALETTE_SIZE];
            }
        }

        ++iter;
    }

    out_iter = iter;
    return 0xFF000000;
}

void newton_cpu(double x_min, double y_min, double x_max, double y_max,
                uint32_t width, uint32_t height,
                uint32_t row_start, uint32_t row_end,
                uint32_t* pixel_buffer, long long& total_iters, bool use_openmp)
{
    double dx = width > 1 ? (x_max - x_min) / (width - 1.0) : 0.0;
    double dy = height > 1 ? (y_max - y_min) / (height - 1.0) : 0.0;

    row_end = std::min(row_end, height);
    long long local_iters = 0;

    if (use_openmp) {
        #pragma omp parallel for reduction(+:local_iters) schedule(dynamic)
        for (int row = (int)row_start; row < (int)row_end; ++row) {
            double cy = y_max - row * dy;
            for (uint32_t col = 0; col < width; ++col) {
                double cx = x_min + col * dx;
                int iter = 0;
                pixel_buffer[(row - row_start) * width + col] = newton_color(cx, cy, iter);
                local_iters += iter;
            }
        }
    } else {
        for (uint32_t row = row_start; row < row_end; ++row) {
            double cy = y_max - row * dy;
            for (uint32_t col = 0; col < width; ++col) {
                double cx = x_min + col * dx;
                int iter = 0;
                pixel_buffer[(row - row_start) * width + col] = newton_color(cx, cy, iter);
                local_iters += iter;
            }
        }
    }

    total_iters = local_iters;
}
