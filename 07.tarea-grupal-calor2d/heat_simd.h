#ifndef HEAT_SIMD_H
#define HEAT_SIMD_H

// Igual que heat_step_serial pero vectorizado con AVX2 sobre double:
// __m256d = 4 doubles por registro (intrínsecas _pd, NO las _ps del fractal).
double heat_step_simd(const double* u, double* u_new, int nx, int ny, double r);

#endif // HEAT_SIMD_H
