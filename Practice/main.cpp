#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "test.h"

int main() {
   
   int w, h, channels;
   uint8_t* rgba_pixels = stbi_load("img.jpg", &w, &h, &channels, STBI_rgb_alpha);
   channels = 4;

   int total_pixeles = w * h;
   uint8_t* gray_pixels = new uint8_t[total_pixeles];

   simd(rgba_pixels, gray_pixels, w, h);

   stbi_write_png("img-gris.png", w, h, STBI_grey, gray_pixels, w);

   return 0;
}
