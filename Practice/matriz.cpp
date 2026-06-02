#include "matriz.h"
#include <immintrin.h>
#include <omp.h>
#include <cmath>

float simdM(float* matriz, int n) {
   __m256 acumulador = _mm256_setzero_ps();
   int limite_simd = n - (n % 8);

   for (int i = 0; i < limite_simd; i+=8) {
      float temp[8];
      for (int p = 0; p < 8; p++) {
         int idx = (i+p) * n + (i+p);
         temp[p] = matriz[idx];
      }
      __m256 d = _mm256_loadu_ps(temp);
      acumulador = _mm256_add_ps(acumulador, d);      
   }
   float result[8];
   _mm256_storeu_ps(result, acumulador);
   float suma = 0.0f;
   for (int i = 0; i < 8; i++) {
      suma += result[i];
   }

   for (int i = limite_simd; i < n; i++) {
      suma += matriz[i*n+i];
   }
   
   return suma;

}

float regionsM(float* matriz, int n) {
   float suma = 0.0f;

   #pragma omp parallel 
   {
      int num_threads = omp_get_num_threads();
      int id_thread = omp_get_thread_num();

      int delta = std::ceil(1.0*n/num_threads);
      int start = id_thread * delta;
      int end = (id_thread + 1) * delta;

      if (end > n) end = n;
      
      float acumulador = 0.0f;
      for (int i = start; i < end; i++) {
         acumulador += matriz[i*n+i];
      }
      #pragma omp atomic
      suma += acumulador;
   }
   return suma;
}