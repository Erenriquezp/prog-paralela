#include "calor_mpi.h"
#include "palette.h"
#include <mpi.h>
#include <algorithm>
#include <cmath>

void calor_mpi(const float* u_antiguo, float* u_nuevo, uint32_t columnas_x, uint32_t filas_y_local,
               float alfa, float paso_tiempo, float paso_espacial_cuadrado,
               uint32_t* buffer_pixeles, double& residuo)
{
    int rank, nproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    float factor = (alfa * paso_tiempo) / paso_espacial_cuadrado;
    double suma_diferencias = 0.0;

    // Resolver stencil para filas locales e interiores
    for (uint32_t j = 1; j <= filas_y_local; j++)
    {
        for (uint32_t i = 0; i < columnas_x; i++)
        {
            uint32_t indice = j * columnas_x + i;

            // Borde superior (Dirichlet)
            if (rank == 0 && j == 1)
            {
                u_nuevo[indice] = 100.0f;
                continue;
            }
            // Borde inferior (Dirichlet)
            if (rank == nproc - 1 && j == filas_y_local)
            {
                u_nuevo[indice] = 0.0f;
                continue;
            }
            // Bordes laterales
            if (i == 0 || i == columnas_x - 1)
            {
                u_nuevo[indice] = 0.0f;
                continue;
            }

            float u_centro = u_antiguo[indice];
            float u_arriba = u_antiguo[indice - columnas_x];
            float u_abajo = u_antiguo[indice + columnas_x];
            float u_izquierda = u_antiguo[indice - 1];
            float u_derecha = u_antiguo[indice + 1];

            float valor_u_nuevo = u_centro + factor * (u_arriba + u_abajo + u_izquierda + u_derecha - 4.0f * u_centro);
            u_nuevo[indice] = valor_u_nuevo;

            float diferencia = valor_u_nuevo - u_centro;
            suma_diferencias += (double)diferencia * diferencia;
        }
    }

    residuo = suma_diferencias;

    // Mapear temperaturas a colores en buffer_pixeles
    for (uint32_t j = 1; j <= filas_y_local; ++j)
    {
        for (uint32_t i = 0; i < columnas_x; ++i)
        {
            uint32_t indice_grilla = j * columnas_x + i;
            float temperatura = u_nuevo[indice_grilla];

            int indice_color = (int)((1.0f - (temperatura / 100.0f)) * (PALETTE_SIZE - 1));
            if (indice_color < 0) indice_color = 0;
            if (indice_color >= PALETTE_SIZE) indice_color = PALETTE_SIZE - 1;

            uint32_t fila_pixel_local = j - 1;
            buffer_pixeles[fila_pixel_local * columnas_x + i] = color_ramp[indice_color];
        }
    }
}