#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//#include "test.h"
//#include "producto.h"
#include "matriz.h"
#include "practice.h"
#include <fmt/core.h>
#include <chrono>

int main() {
   // int w, h, channels;
   // uint8_t* rgba = stbi_load("img.jpg", &w, &h, &channels, STBI_rgb_alpha);

   // int total_bytes = w * h * 4;
   // uint8_t* negative_pixels = new uint8_t[total_bytes];

   // simdP(rgba, negative_pixels, w, h);
   // stbi_write_png("salida_sepia.png", w, h, STBI_rgb_alpha, negative_pixels, w*4);   

   const int n = 4;
   float m[n*n] = {
      1,1,1,1,
      1,1,1,1,
      1,1,1,1,
      1,1,1,1
   };

   float suma_diagonal = regionsM(m, n);
   //float suma_diagonal = simdM(m, n);
   fmt::println("suma= {}", suma_diagonal);
}
