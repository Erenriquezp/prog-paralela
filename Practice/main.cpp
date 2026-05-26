#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// #include "test.h"
#include "producto.h"
#include <fmt/core.h>
#include <chrono>

int main() {
   
   // int w, h, channels;
   // uint8_t* rgba_pixels = stbi_load("img.jpg", &w, &h, &channels, STBI_rgb_alpha);
   // channels = 4;

   // int total_pixeles = w * h;
   // uint8_t* gray_pixels = new uint8_t[total_pixeles];

   // simd(rgba_pixels, gray_pixels, w, h);

   // stbi_write_png("img-gris.png", w, h, STBI_grey, gray_pixels, w);

   const int N = 16;

   float x[N] = {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1};
   float y[N] = {2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2};

   auto start = std::chrono::high_resolution_clock::now();
   float s = simd(x, y, N);
   auto end = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double, std::milli> elapsed = end - start;
   fmt::println("simd: {}, time: {:.4f}", s, (elapsed.count()));

   auto start2 = std::chrono::high_resolution_clock::now();
   float r = simd(x, y, N);
   auto end2 = std::chrono::high_resolution_clock::now();

   std::chrono::duration<double, std::milli> elapsed2 = end2 - start2;
   fmt::println("regiones: {}, time: {:.6f}", r, (elapsed2.count()));
   return 0;
}
