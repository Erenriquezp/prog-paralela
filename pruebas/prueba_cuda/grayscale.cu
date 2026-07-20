#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cuda_runtime.h>
#include <fmt/core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define CHECK(expr) {                               \
        auto internal_error = (expr);               \
        if (internal_error != cudaSuccess) {        \
            fmt::print("CUDA Error: {} at line {}", cudaGetErrorString(internal_error), __LINE__); \
            exit(EXIT_FAILURE);                     \
        }                                           \
    }

__global__ void escala_grises_kernel(uint32_t width, uint32_t height, uint32_t *buffer_entrada, uint32_t *buffer_salida) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index < width * height) {
        uint32_t pixel = buffer_entrada[index];

        unsigned char a = (pixel >> 24) & 0xFF;
        unsigned char r = (pixel >> 16) & 0xFF;
        unsigned char g = (pixel >> 8)  & 0xFF;
        unsigned char b = pixel         & 0xFF;

        unsigned char gris = (unsigned char)(0.299f * r + 0.587f * g + 0.114f * b);

        buffer_salida[index] = (a << 24) | (gris << 16) | (gris << 8) | gris;
    }
}

int main() {
    
    std::string filename = "image01.jpg";
    int img_width = 0;
    int img_height = 0;
    int canales = 0;

    unsigned char* raw_pixels = stbi_load(filename.c_str(), &img_width, &img_height, &canales, STBI_rgb_alpha);
    
    size_t total_pixels = img_width * img_height;
    size_t buffer_size = total_pixels * sizeof(uint32_t);

    uint32_t *device_input_buffer = nullptr;
    uint32_t *device_output_buffer = nullptr;

    CHECK(cudaMalloc(&device_input_buffer, buffer_size));
    CHECK(cudaMalloc(&device_output_buffer, buffer_size));

    CHECK(cudaMemcpy(device_input_buffer, raw_pixels, buffer_size, cudaMemcpyHostToDevice));

    int threads_per_block = 256;
    int blocks_per_grid = (total_pixels + threads_per_block - 1) / threads_per_block;
    
    escala_grises_kernel<<<blocks_per_grid, threads_per_block>>>(
        img_width, img_height, 
        device_input_buffer, 
        device_output_buffer
    );
    CHECK(cudaGetLastError());

    CHECK(cudaDeviceSynchronize());

    CHECK(cudaMemcpy(raw_pixels, device_output_buffer, buffer_size, cudaMemcpyDeviceToHost));

    std::string output_filename = "resultado_gris.png";
    stbi_write_png(output_filename.c_str(), img_width, img_height, 4, raw_pixels, img_width * 4);

    stbi_image_free(raw_pixels);
    CHECK(cudaFree(device_input_buffer));
    CHECK(cudaFree(device_output_buffer));

    return 0;
}