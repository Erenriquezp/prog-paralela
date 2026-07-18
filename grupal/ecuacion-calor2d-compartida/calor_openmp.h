#ifndef CALOR_OPENMP_H
#define CALOR_OPENMP_H

#include <cstdint>

void calor_openmp_regiones(const float* u_antiguo, float* u_nuevo, uint32_t columnas_x, uint32_t filas_y,
                           float alfa, float paso_tiempo, float paso_espacial_cuadrado,
                           uint32_t* buffer_pixeles, double& residuo);

#endif