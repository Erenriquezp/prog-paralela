#include "calor_serial.h"
#include "palette.h"
#include <cmath>
#include <algorithm>

void calor_serial(const float* u_antiguo, float* u_nuevo, uint32_t columnas_x, uint32_t filas_y,
                  float alfa, float paso_tiempo, float paso_espacial_cuadrado,
                  uint32_t* buffer_pixeles, double& residuo)
{
    float factor = (alfa * paso_tiempo) / paso_espacial_cuadrado;
    double suma_diferencias = 0.0;

    // 1. Calcular stencil para celdas interiores
    for (uint32_t j = 1; j < filas_y - 1; ++j)
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

    residuo = std::sqrt(suma_diferencias / (columnas_x * filas_y));

    // 2. Mapear temperaturas a colores de píxeles
    for (uint32_t j = 0; j < filas_y; ++j)
    {
        for (uint32_t i = 0; i < columnas_x; ++i)
        {
            uint32_t indice = j * columnas_x + i;
            float temperatura = u_nuevo[indice];

            // Normalizar temperatura de [0, 100] al rango de la paleta [0, PALETTE_SIZE - 1]
            int indice_color = (int)((1.0f - (temperatura / 100.0f)) * (PALETTE_SIZE - 1));
            if (indice_color < 0) indice_color = 0;
            if (indice_color >= PALETTE_SIZE) indice_color = PALETTE_SIZE - 1;

            buffer_pixeles[indice] = color_ramp[indice_color];
        }
    }
}