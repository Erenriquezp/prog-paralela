#include "producto_escalar.h"
#include <omp.h>
#include <immintrin.h>
#include <cmath>

float producto_escalar_simd(const float* X, const float* Y, uint32_t n) {
    __m256 acumulador = _mm256_setzero_ps(); 

    for (uint32_t i = 0; i < n; i += 8) {
        __m256 vecX = _mm256_loadu_ps(&X[i]);
        __m256 vecY = _mm256_loadu_ps(&Y[i]);

        __m256 prod = _mm256_mul_ps(vecX, vecY); 
        acumulador = _mm256_add_ps(acumulador, prod);
    }

    float resultados_parciales[8];
    _mm256_storeu_ps(resultados_parciales, acumulador);

    float total = 0.0f;
    for (int i = 0; i < 8; ++i) {
        total += resultados_parciales[i];
    }

    return total;
}

float producto_escalar_regiones(const float* X, const float* Y, uint32_t n) {
    float suma_total = 0.0f;

    #pragma omp parallel
    {
        int thread_count = omp_get_num_threads();
        int thread_id = omp_get_thread_num();

        int delta = std::ceil(n * 1.0 / thread_count);
        int start = thread_id * delta;
        int end = (thread_id + 1) * delta;

        if (thread_id == thread_count - 1) {
            end = n;
        }

        float acumulador_local = 0.0f;
        for (int i = start; i < end; ++i) {
            acumulador_local += X[i] * Y[i];
        }

        suma_total += acumulador_local;
    }

    return suma_total;
}