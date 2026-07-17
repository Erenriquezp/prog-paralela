#include <cuda_runtime.h>
#include <cmath>
#include <cstdint>
#include "palette.h"

// Memoria constante en la GPU para la paleta
__constant__ unsigned int d_color_ramp[PALETTE_SIZE];
// Función auxiliar ejecutada puramente en el dispositivo de la GPU
__device__
uint32_t acotado(double x, double y, double cr, double ci, int max_iteraciones) {

    int iter = 1;
    double zr = x;
    double zi = y;

    while (iter < max_iteraciones && (zr * zr + zi * zi) < 4.0) {
        double dr = zr * zr - zi * zi + cr;
        double di = 2.0 * zr * zi + ci;
        zr = dr;
        zi = di;
        
        iter++;
    }

    if (iter < max_iteraciones) {
        // La norma es mayor que 2 (escapó), asignamos color de la paleta
        int index = iter % PALETTE_SIZE;
        return d_color_ramp[index];
    } 

    return 0x000000FF; // CORRECCIÓN: Negro en formato SFML RGBA estándar (0xRRGGBBAA)
}

__global__
void julia_kernel(double centro_real, double centro_img, int num_iteraciones, 
                  double x_min, double y_min, double x_max, double y_max, 
                  uint32_t width, uint32_t height, uint32_t* pixel_buffer) { 

    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    int index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index < width * height) {
        uint32_t i = index % width; // Columna
        uint32_t j = index / width; // Fila

        // Coordenadas en el plano complejo mapeadas desde la pantalla
        double x = x_min + i * dx;
        double y = y_max - j * dy; 

        // Evaluamos el píxel y guardamos el color en el buffer indexado linealmente
        auto color = acotado(x, y, centro_real, centro_img, num_iteraciones);
        pixel_buffer[index] = color; // Es más rápido usar el 'index' ya calculado
    }
}

// Función para inicializar la paleta desde C++
extern "C" void copiar_paleta(unsigned int* h_palette) {
    cudaMemcpyToSymbol(d_color_ramp, h_palette, PALETTE_SIZE * sizeof(unsigned int));
}

// Función contenedora expuesta al main.cpp
extern "C" void julia_gpu(double centro_real, double centro_img, int num_iteraciones, 
                          double x_min, double y_min, double x_max, double y_max, 
                          uint32_t WIDTH, uint32_t HEIGHT, uint32_t* pixel_buffer) {

    int threads_per_block = 1024;
    int blocks_per_grid = (WIDTH * HEIGHT + threads_per_block - 1) / threads_per_block;

    julia_kernel<<<blocks_per_grid, threads_per_block>>>(
        centro_real, centro_img, num_iteraciones, 
        x_min, y_min, x_max, y_max, 
        WIDTH, HEIGHT, pixel_buffer
    );
}