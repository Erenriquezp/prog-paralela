#include "practice.h"
#include <omp.h>
#include <immintrin.h>
#include <cmath>

void simdP(const uint8_t* rgba, uint8_t* sepia, int w, int h) {
   // $$R' = 0.393 \cdot R + 0.769 \cdot G + 0.189 \cdot B$$
   int total_pixels = w * h;
   int limite_simd = total_pixels - (total_pixels % 8);

   __m256 sr = _mm256_set1_ps(0.393f);
   __m256 sg = _mm256_set1_ps(0.769f);
   __m256 sb = _mm256_set1_ps(0.189f);

   for (int i = 0; i < limite_simd; i+=8) {
      float temp_r[8], temp_g[8], temp_b[8];
      for (int p = 0; p < 8; p++) {
         int idx = (i + p) * 4;
         temp_r[p] = rgba[idx];
         temp_g[p] = rgba[idx + 1];
         temp_b[p] = rgba[idx + 2];
      }
      __m256 r = _mm256_loadu_ps(temp_r);
      __m256 g = _mm256_loadu_ps(temp_g);
      __m256 b = _mm256_loadu_ps(temp_b);

      __m256 nr = _mm256_mul_ps(r, sr);
      __m256 ng = _mm256_mul_ps(g, sg);
      __m256 nb = _mm256_mul_ps(b, sb);

      __m256 s = _mm256_add_ps(_mm256_add_ps(nb,ng),nr);

      float result[8];
      _mm256_storeu_ps(result, s);

     for (int p = 0; p < 8; p++) {
         int out_idx = (i + p) * 4;
         
         float valor_r = result[p];
         if (valor_r > 255.0f) valor_r = 255.0f;

         sepia[out_idx]     = (uint8_t)valor_r;      // Nuevo R modificado
         sepia[out_idx + 1] = rgba[out_idx + 1];     // G se mantiene original (o cambia si pide el enunciado)
         sepia[out_idx + 2] = rgba[out_idx + 2];     // B se mantiene original
         sepia[out_idx + 3] = 255;                   // Alpha opaco fijo
      }   
   } 
}

void regionsP(const uint8_t* rgba, uint8_t* sepia, int w, int h);