#include "filtro.h"
#include <omp.h>
#include <immintrin.h>
#include <cmath>

void filtro_escala_grises_simd(const uint8_t* rgba_pixels, uint8_t* gray_pixels, int width, int height) {
   int total_pixeles = width * height;

   __m256 wR = _mm256_set1_ps(0.21f);
   __m256 wG = _mm256_set1_ps(0.72f);
   __m256 wB = _mm256_set1_ps(0.07f);

   int limite_simd = total_pixeles - (total_pixeles % 8);

   for (int i = 0; i < limite_simd; i += 8) {
      float r_arr[8], g_arr[8], b_arr[8];

      for (int p = 0; p < 8; p++) {
         int idx = (i + p) * 4;
         r_arr[p] = (float)rgba_pixels[idx];
         g_arr[p] = (float)rgba_pixels[idx + 1];
         b_arr[p] = (float)rgba_pixels[idx + 2];
      }

      __m256 vr = _mm256_loadu_ps(r_arr);
      __m256 vg = _mm256_loadu_ps(g_arr);
      __m256 vb = _mm256_loadu_ps(b_arr);
      
      __m256 res_r = _mm256_mul_ps(vr, wR);
      __m256 res_g = _mm256_mul_ps(vg, wG);
      __m256 res_b = _mm256_mul_ps(vb, wB);
      __m256 resultado = _mm256_add_ps(_mm256_add_ps(res_r, res_g), res_b);

      float res_gray[8];
      _mm256_storeu_ps(res_gray, resultado);

      for (int p = 0; p < 8; p++) {
         gray_pixels[i + p] = (uint8_t)res_gray[p];
      }
   }

   for (int i = limite_simd; i < total_pixeles; i++) {
      int idx = i * 4;
      uint8_t r = rgba_pixels[idx];
      uint8_t g = rgba_pixels[idx + 1];
      uint8_t b = rgba_pixels[idx + 2];

      gray_pixels[i] = (uint8_t)(0.21f * r + 0.72f * g + 0.07f * b);
   }
}

void filtro_escala_grises_regiones(const uint8_t* rgba_pixels, uint8_t* gray_pixels, int width, int height) {
   int total_pixels = width * height;

   #pragma omp parallel
   {
      int thread_count = omp_get_num_threads();
      int thread_id = omp_get_thread_num();

      int delta = std::ceil(total_pixels * 1.0 / thread_count);
      int start = thread_id * delta;
      int end = (thread_id + 1) * delta;

      if (thread_id == thread_count - 1) {
         end = total_pixels;
      }
      
      for (int i = start; i < end; i++) {
         int rgba_idx = i * 4;
         uint8_t r = rgba_pixels[rgba_idx];
         uint8_t g = rgba_pixels[rgba_idx + 1];
         uint8_t b = rgba_pixels[rgba_idx + 2];

         gray_pixels[i] = (uint8_t)(0.21f * r + 0.72f * g + 0.07f * b);
      }
   }
}
