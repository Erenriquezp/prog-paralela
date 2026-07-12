#include "heat_simd.h"

#include <cmath>
#include <immintrin.h> // AVX2: __m256d procesa 4 doubles a la vez

double heat_step_simd(const double* u, double* u_new, int nx, int ny, double r) {
    // ============================================================
    // TODO (Integrante C) — sección 5.2 del PLAN.md.
    // Referencia de patrón: 001.fractal-Julia/fractal_simd.cpp, pero
    // aquí todo es _pd (double, 4 carriles), no _ps (float, 8 carriles).
    //
    // 1. Constantes vectoriales fuera de los bucles:
    //       __m256d r_vec  = _mm256_set1_pd(r);
    //       __m256d cuatro = _mm256_set1_pd(4.0);
    //       __m256d acum   = _mm256_setzero_pd();   // residuo vectorial
    //
    // 2. Por cada fila interior j (1 .. ny-2):
    //    bucle vectorizado  for (i = 1; i + 3 <= nx-2; i += 4):
    //       - cargar con _mm256_loadu_pd (loadu porque i±1 desalinea):
    //           centro, izq (idx-1), der (idx+1),
    //           arriba ((j-1)*nx+i), abajo ((j+1)*nx+i)
    //       - vecinos = izq + der + arriba + abajo - cuatro*centro
    //         (con _mm256_add_pd / _mm256_sub_pd / _mm256_mul_pd)
    //       - nuevo = centro + r_vec * vecinos  ->  _mm256_storeu_pd(&u_new[idx], nuevo)
    //       - diff = nuevo - centro;  acum += diff * diff
    //
    // 3. Resto escalar: las últimas (nx-2) % 4 celdas de cada fila se
    //    procesan con el código serial (mismas fórmulas, sin intrínsecas).
    //    ¡No olvidar acumular también su residuo!
    //
    // 4. Suma horizontal de acum al final (mismo truco que el
    //    _mm256_storeu_ps(d, mk) del fractal, pero con double d[4]):
    //       double d[4]; _mm256_storeu_pd(d, acum);
    //       suma += d[0] + d[1] + d[2] + d[3];
    //
    // 5. return std::sqrt(suma / (nx*ny));
    //
    // VERIFICACIÓN: el residuo debe coincidir con el serial (puede
    // diferir en los últimos decimales por reasociación de la suma —
    // anotarlo para el informe).
    // ============================================================

    (void)u; (void)u_new; (void)nx; (void)ny; (void)r; // quitar al implementar
    return 0.0;
}
