#include <cuda_runtime.h>
#include <cmath>
#include <cstdint>
#include "palette.h"

#define NBINS 16

__constant__ unsigned int d_color_ramp[PALETTE_SIZE];

__device__
uint32_t burning_ship_pixel(double cx, double cy, int max_iter, int& out_iter)
{
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < max_iter && (zr * zr + zi * zi) <= 4.0) {
        double ar = fabs(zr);
        double ai = fabs(zi);

        double next_zr = ar * ar - ai * ai + cx;
        double next_zi = 2.0 * ar * ai + cy;

        zr = next_zr;
        zi = next_zi;
        ++iter;
    }

    out_iter = iter;

    if (iter >= max_iter) {
        return 0xFF000000;
    }

    return d_color_ramp[iter % PALETTE_SIZE];
}

__global__
void burning_ship_kernel(int max_iter,
                         double x_min, double y_min, double x_max, double y_max,
                         uint32_t width, uint32_t height,
                         uint32_t* pixel_buffer, int* d_hist)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index < width * height) {
        uint32_t col = index % width;
        uint32_t row = index / width;

        double dx = (x_max - x_min) / width;
        double dy = (y_max - y_min) / height;

        double cx = x_min + col * dx;
        double cy = y_max - row * dy;

        int iter = 0;
        pixel_buffer[index] = burning_ship_pixel(cx, cy, max_iter, iter);

        if (iter < max_iter) {
            int bin = iter * NBINS / max_iter;
            if (bin >= NBINS) bin = NBINS - 1;
            atomicAdd(&d_hist[bin], 1);
        }
    }
}

extern "C" void copiar_paleta(unsigned int* h_palette)
{
    cudaMemcpyToSymbol(d_color_ramp, h_palette, PALETTE_SIZE * sizeof(unsigned int));
}

extern "C" void burning_ship_gpu(int max_iter,
                                  double x_min, double y_min, double x_max, double y_max,
                                  uint32_t width, uint32_t height,
                                  uint32_t* device_pixels, int* d_hist)
{
    cudaMemset(d_hist, 0, NBINS * sizeof(int));

    int threads_per_block = 1024;
    int blocks = (width * height + threads_per_block - 1) / threads_per_block;

    burning_ship_kernel<<<blocks, threads_per_block>>>(
        max_iter, x_min, y_min, x_max, y_max,
        width, height, device_pixels, d_hist
    );
}
