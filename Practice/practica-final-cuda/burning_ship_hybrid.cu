#include <cuda_runtime.h>
#include <cstdint>
#include <cmath>
#include "palette.h"

#define NBINS 16

__constant__ unsigned int d_color_ramp[PALETTE_SIZE];

__device__ 
uint32_t pixel(double cx, double cy, int max_iter, &out_iter) {
   double zr = 0.0;
   double zi = 0.0;
   int iter = 0;
   while (iter < max_iter && (zr*zr + zi*zi) <= 4.0) {
      double ar = fabs(zr);
      double ai = fabs(zi);

      double next_zr = ar*ar - ai*ai + cx;
      double next_zi = 2.0 * ar * ai + cy;

      zr = next_zr;
      zi = next_zi;
      ++iter;
   }

   out_iter = iter;

   if (iter >= max_iter) {
      return 0xFF000000;
   }
   return d_color_ramp[iter % PALETTE_SIZE]
}

__global__ void kernel(double max_iter, double x_min, double y_min, double x_max, double y_max,
                        uint32_t width, uint32_t height, uint32_t *pixel_buffer, int *d_hist) {
   int index = blockIdx.x * blockDim.x + threadIdx.x;
   if (index < width * height) {
      double col = (index) % width;
      double row = (index) / width;
      double dx = (x_max - x_mix) / width;
      double dy = (y_max - y_min) / height;
      double cx = x_min + col * dx;
      double cy = y_max + row * dy;
      pixel_buffer[index] = pixel(cx, cy, max_iter, iter);
   }
   
}

extern "C" void copiar_paleta(uint32_t *palette_h) {
   cudaMemcpyToSimbol(d_color_ramp, pallete_h, PALETTE_SIZE*sizeof(uint32_t));
}

extern "C" void burning_ship(double max_iter, double x_min, double y_min, double x_max, double y_max,
                        uint32_t width, uint32_t height, uint32_t *pixel_buffer, int *d_hist) {
   int threads = 1024;
   int blocks = (threads*width*height -1) / threads;
   kernel<<<blocks, threads>>>(max_iter, x_min, y_min, x_max, y_max,
                              width, height, pixel_buffer, d_hist);
}