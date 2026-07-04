#include "producto.h"
#include <omp.h>
#include <immintrin.h>
#include <cmath>

float simd(float* x, float* y, int n) {
   int limite_simd = n - (n % 8);
   __m256 acumulador = _mm256_setzero_ps();

   for (int i = 0; i < limite_simd; i+= 8) {
      __m256 vx = _mm256_loadu_ps(&x[i]);
      __m256 vy = _mm256_loadu_ps(&y[i]);

      __m256 mul = _mm256_mul_ps(vx, vy);
      acumulador = _mm256_add_ps(acumulador, mul);
   }

   float res[8];
   _mm256_storeu_ps(res, acumulador);
   float suma = 0.0f;
   for (int i = 0; i < 8; i++) {
      suma += res[i];
   }
   for (int i = limite_simd; i < n; i++) {
      suma += x[i] * y[i];
   }
   
   return suma;
}

float regiones(float* x, float* y, int n) {
   float suma = 0.0f;

   #pragma omp parallel
   {
      int num_threads = omp_get_num_threads();
      int id_thread = omp_get_thread_num();

      int delta = std::ceil(n * 1.0f/num_threads);
      int start = id_thread * delta;
      int end = (id_thread + 1) * delta;

      if (end > n) {
         end = n;
      }

      float sum_temp = 0.0f;
      for (int i = start; i < end; i++) {
         sum_temp += x[i] * y[i];
      }
      #pragma omp atomic
      suma += sum_temp;
   }
   return suma;
}
