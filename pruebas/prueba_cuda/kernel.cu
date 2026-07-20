#include <cuda_runtime.h>
#include <cmath>
#include <cstdint>

__constant__ int borde_kernel[9] = {
    -1, -1, -1, 
    -1,  8, -1, 
    -1, -1, -1
}; 

__device__ void extraer_pixel(uint32_t pixel, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) {
    a = (pixel >> 24) & 0xFF;
    r = (pixel >> 16) & 0xFF;
    g = (pixel >> 8)  & 0xFF;
    b = pixel         & 0xFF;
}

__device__ uint32_t empaquetar_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

__global__ void realzar_bordes_kernel(uint32_t width, uint32_t height, uint32_t *buffer_entrada, uint32_t *buffer_salida) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index < width * height) {
        int x = index % width;
        int y = index / width;

        if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
            buffer_salida[index] = buffer_entrada[index];
            return;
        }

        int resultado_r = 0, resultado_g = 0, resultado_b = 0;

        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                int vecino_x = x + kx;
                int vecino_y = y + ky;
                int vecino_index = vecino_y * width + vecino_x;

                uint32_t vecino_pixel = buffer_entrada[vecino_index];

                unsigned char r, g, b, a;
                extraer_pixel(vecino_pixel, r, g, b, a);

                int kernel_idx = (ky + 1) * 3 + (kx + 1); 
                int kernel_value = borde_kernel[kernel_idx];

                resultado_r += r * kernel_value;
                resultado_g += g * kernel_value;
                resultado_b += b * kernel_value;
            }
        }

        resultado_r = max(0, min(255, resultado_r));
        resultado_g = max(0, min(255, resultado_g));
        resultado_b = max(0, min(255, resultado_b));

        unsigned char orig_r, orig_g, orig_b, orig_a;
        extraer_pixel(buffer_entrada[index], orig_r, orig_g, orig_b, orig_a);

        buffer_salida[index] = empaquetar_pixel(resultado_r, resultado_g, resultado_b, orig_a);
    }
}

extern "C" void aplicar_realzamiento(uint32_t width, uint32_t height, uint32_t *d_buffer_entrada, uint32_t *d_buffer_salida) {
    int threads_per_block = 256;
    int blocks_per_grid = (width * height + threads_per_block - 1) / threads_per_block;
    
    realzar_bordes_kernel<<<blocks_per_grid, threads_per_block>>>(
        width, height,
        d_buffer_entrada,
        d_buffer_salida
    );
}