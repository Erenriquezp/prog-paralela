#include "calor_openmp.h"
#include "palette.h"
#include <omp.h>
#include <cmath>
#include <algorithm>

void calor_openmp_regiones(const float* u_antiguo, float* u_nuevo, uint32_t columnas_x, uint32_t filas_y,
                           float alfa, float paso_tiempo, float paso_espacial_cuadrado,
                           uint32_t* buffer_pixeles, double& residuo)
{
    float factor = (alfa * paso_tiempo) / paso_espacial_cuadrado;
    double suma_diferencias = 0.0;

    // 1. Resolver stencil en celdas interiores usando división manual de regiones de OpenMP
#pragma omp parallel reduction(+:suma_diferencias)
    {
        int cantidad_hilos = omp_get_num_threads();
        int id_hilo = omp_get_thread_num();

        int total_filas_interiores = filas_y - 2;
        int bloque_filas = total_filas_interiores / cantidad_hilos;
        int residuo_filas = total_filas_interiores % cantidad_hilos;

        int inicio_j = 1 + id_hilo * bloque_filas + std::min(id_hilo, residuo_filas);
        int fin_j = inicio_j + bloque_filas + (id_hilo < residuo_filas ? 1 : 0);

        for (int j = inicio_j; j < fin_j; ++j)
        {
            for (uint32_t i = 1; i < columnas_x - 1; ++i)
            {
                uint32_t indice = j * columnas_x + i;
                float u_centro = u_antiguo[indice];
                float u_arriba = u_antiguo[(j - 1) * columnas_x + i];
                float u_abajo = u_antiguo[(j + 1) * columnas_x + i];
                float u_izquierda = u_antiguo[indice - 1];
                float u_derecha = u_antiguo[indice + 1];

                float valor_u_nuevo = u_centro + factor * (u_arriba + u_abajo + u_izquierda + u_derecha - 4.0f * u_centro);
                u_nuevo[indice] = valor_u_nuevo;

                float diferencia = valor_u_nuevo - u_centro;
                suma_diferencias += (double)diferencia * diferencia;
            }
        }
    }

    // Calcular residuo norma L2
    residuo = std::sqrt(suma_diferencias / (columnas_x * filas_y));

    // 2. Mapear temperaturas a colores de píxeles usando división manual de regiones de OpenMP
#pragma omp parallel
    {
        int cantidad_hilos = omp_get_num_threads();
        int id_hilo = omp_get_thread_num();

        int bloque_filas = filas_y / cantidad_hilos;
        int residuo_filas = filas_y % cantidad_hilos;

        int inicio_j = id_hilo * bloque_filas + std::min(id_hilo, residuo_filas);
        int fin_j = inicio_j + bloque_filas + (id_hilo < residuo_filas ? 1 : 0);

        for (int j = inicio_j; j < fin_j; ++j)
        {
            for (uint32_t i = 0; i < columnas_x; ++i)
            {
                uint32_t indice = j * columnas_x + i;
                float temperatura = u_nuevo[indice];

                int indice_color = (int)((1.0f - (temperatura / 100.0f)) * (PALETTE_SIZE - 1));
                if (indice_color < 0) indice_color = 0;
                if (indice_color >= PALETTE_SIZE) indice_color = PALETTE_SIZE - 1;

                buffer_pixeles[indice] = color_ramp[indice_color];
            }
        }
    }
}
