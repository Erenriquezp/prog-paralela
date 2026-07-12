#ifndef HEAT_OPENMP_H
#define HEAT_OPENMP_H

// Variante 1: regiones paralelas manuales (reparto de filas por bloques,
// como julia_openmp_regiones). Es la variante principal del enunciado
// ("#pragma omp parallel, paralelización manual").
double heat_step_openmp_regiones(const double* u, double* u_new, int nx, int ny, double r);

// Variante 2: #pragma omp parallel for + reduction(+:suma) — la forma
// idiomática de la reducción, para contrastar en el informe.
double heat_step_openmp_for(const double* u, double* u_new, int nx, int ny, double r);

#endif // HEAT_OPENMP_H
