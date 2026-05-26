#include "test.h"
#include <omp.h>
#include <immintrin.h>
#include <cmath>

void simd(const uint8_t* entrada, uint8_t* salida, int w, int h) {
   int total_pixels = w * h;

   __m256 lr = _mm256_set1_ps(0.21f);
   __m256 lg = _mm256_set1_ps(0.72f);
   __m256 lb = _mm256_set1_ps(0.07f);

   for (int i = 0; i < total_pixels; i+=8) {
      float r_temp[8], g_temp[8], b_temp[8];

      for (int p = 0; p < 8; p++) {
         int idx = (i + p) * 4;
         r_temp[p] = entrada[idx];
         g_temp[p] = entrada[idx+1];
         b_temp[p] = entrada[idx+2];
      }
      __m256 vr = _mm256_loadu_ps(r_temp);
      __m256 vg = _mm256_loadu_ps(g_temp);
      __m256 vb = _mm256_loadu_ps(b_temp);
      
      __m256 r = _mm256_mul_ps(vr, lr);
      __m256 g = _mm256_mul_ps(vg, lg);
      __m256 b = _mm256_mul_ps(vb, lb);

      __m256 resultado = _mm256_add_ps(_mm256_add_ps(b,g),r);

      float res_final[8];
      _mm256_storeu_ps(res_final, resultado);

      for (int p = 0; p < 8; p++) {
         salida[i+p] = res_final[p];
      }
   }
}

void regiones(const uint8_t* entrada, uint8_t* salida, int w, int h) {
   int total_pixels = w * h;

   #pragma omp parallel
   {
      int total_threads = omp_get_num_threads();
      int id_thread = omp_get_thread_num();

      int delta = (total_pixels + total_threads - 1) / total_threads;
      int start = id_thread * delta;
      int end = (id_thread + 1) * delta;

      if (end > total_pixels) {
         end = total_pixels;
      }

      for (int i = start; i < end; i++) {
         int idx = i * 4;
         uint8_t r = entrada[idx];
         uint8_t g = entrada[idx + 1];
         uint8_t b = entrada[idx + 2];

         salida[i] = (uint8_t)(0.21f * r + 0.72f * g + 0.07 * b);
      }
   }
}
