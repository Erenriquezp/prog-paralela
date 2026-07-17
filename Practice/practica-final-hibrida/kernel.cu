#include <cuda_runtime.h>
#include <cmath>
#include <cstdint>
#include "palette.h"

__constant__ unsigned int d_color_ramp[PALETTE_SIZE];

static const double EPS = 1e-4;
__device__ static const double roots_r[3] = { 1.0, -0.5, -0.5 };
__device__ static const double roots_i[3] = { 0.0, 0.8660254037844386, -0.8660254037844386 };

__device__
uint32_t newton_pixel_gpu(double cx, double cy, int max_iter, int& out_iter)
{
    double zr = cx;
    double zi = cy;
    int iter = 0;

    while (iter < max_iter) {
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
                return d_color_ramp[(k * 5 + iter) % PALETTE_SIZE];
            }
        }

        ++iter;
    }

    out_iter = iter;
    return 0xFF000000;
}

__global__
void newton_kernel(int max_iter,
                   double x_min, double y_min, double x_max, double y_max,
                   uint32_t width, uint32_t height,
                   uint32_t row_start, uint32_t row_end,
                   uint32_t* pixel_buffer, unsigned long long* d_iters)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t num_pixels = width * (row_end - row_start);

    if (idx < num_pixels) {
        uint32_t col = idx % width;
        uint32_t row = row_start + (idx / width);

        double dx = (x_max - x_min) / width;
        double dy = (y_max - y_min) / height;

        double cx = x_min + col * dx;
        double cy = y_max - row * dy;

        int iter = 0;
        pixel_buffer[idx] = newton_pixel_gpu(cx, cy, max_iter, iter);

        atomicAdd(d_iters, (unsigned long long)iter);
    }
}

extern "C" void copiar_paleta(unsigned int* h_palette)
{
    cudaMemcpyToSymbol(d_color_ramp, h_palette, PALETTE_SIZE * sizeof(unsigned int));
}

extern "C" void newton_gpu(int max_iter,
                           double x_min, double y_min, double x_max, double y_max,
                           uint32_t width, uint32_t height,
                           uint32_t row_start, uint32_t row_end,
                           uint32_t* device_pixels, long long* d_iters,
                           uint32_t* host_pixels, long long& total_iters)
{
    unsigned long long h_iters_temp = 0;
    cudaMemcpy(d_iters, &h_iters_temp, sizeof(unsigned long long), cudaMemcpyHostToDevice);

    uint32_t num_pixels = width * (row_end - row_start);
    int threads_per_block = 256;
    int blocks = (num_pixels + threads_per_block - 1) / threads_per_block;

    newton_kernel<<<blocks, threads_per_block>>>(
        max_iter, x_min, y_min, x_max, y_max,
        width, height, row_start, row_end,
        device_pixels, (unsigned long long*)d_iters
    );
    cudaDeviceSynchronize();

    cudaMemcpy(host_pixels, device_pixels, num_pixels * sizeof(uint32_t), cudaMemcpyDeviceToHost);
    cudaMemcpy(&h_iters_temp, d_iters, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
    total_iters = (long long)h_iters_temp;
}
