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
      float temp_r[8], temp_g[8], temp_b[8]; 
      for (int p = 0; p < 8; p++) {
         int idx = (p + i) * 4;
         temp_r[p] = entrada[idx];
         temp_g[p] = entrada[idx + 1];
         temp_b[p] = entrada[idx + 2];
      }
      __m256 vr = _mm256_loadu_ps(temp_r);
      __m256 vg = _mm256_loadu_ps(temp_g);
      __m256 vb = _mm256_loadu_ps(temp_b);

      __m256 res_r = _mm256_mul_ps(vr, lr);
      __m256 res_g = _mm256_mul_ps(vg, lg);
      __m256 res_b = _mm256_mul_ps(vb, lb);

      __m256 result = _mm256_add_ps(_mm256_add_ps(res_b, res_g), res_b);

      float res_final[8];
      _mm256_storeu_ps(res_final, result);

      for (int p = 0; p < 8; p++) {
         salida[i + p] = res_final[p];
      }
   }
}

void regiones(const uint8_t* entrada, uint8_t* salida, int w, int h) {
   int total = w * h;
   #pragma omp parallel
   {
      int total_threads = omp_get_num_threads();
      int id_thread = omp_get_thread_num();

      int delta = (total_threads + total)/total_threads;
      int start = id_thread * delta;
      int end = (id_thread + 1) * delta;

      if (id_thread == total_threads - 1) {
         end = total;
      }

      for (int i = start; i < end; i++) {
         int idx = (i * 4);
         uint8_t r = entrada[idx];
         uint8_t g = entrada[idx + 1];
         uint8_t b = entrada[idx + 2];

         salida[i] = r * 0.21f + g * 0.72f + 0.07f * b;
      } 
   }
}