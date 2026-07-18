#include "calor_simd.h"
#include "palette.h"
#include <immintrin.h>
#include <cmath>
#include <algorithm>

void calor_simd(const float* u_antiguo, float* u_nuevo, uint32_t columnas_x, uint32_t filas_y,
                float alfa, float paso_tiempo, float paso_espacial_cuadrado,
                uint32_t* buffer_pixeles, double& residuo)
{
    float factor = (alfa * paso_tiempo) / paso_espacial_cuadrado;
    double suma_diferencias = 0.0;

    __m256 vec_factor = _mm256_set1_ps(factor);
    __m256 vec_cuatro = _mm256_set1_ps(4.0f);


    for (uint32_t j = 1; j < filas_y - 1; ++j)
    {
        __m256 suma_dif_al_cuadrado = _mm256_setzero_ps();
        uint32_t i = 1;

        // Procesar de a 8 elementos simultáneamente
        for (; i + 8 <= columnas_x - 1; i += 8)
        {
            __m256 u_centro = _mm256_loadu_ps(&u_antiguo[j * columnas_x + i]);
            __m256 u_arriba = _mm256_loadu_ps(&u_antiguo[(j - 1) * columnas_x + i]);
            __m256 u_abajo  = _mm256_loadu_ps(&u_antiguo[(j + 1) * columnas_x + i]);
            __m256 u_izquierda = _mm256_loadu_ps(&u_antiguo[j * columnas_x + (i - 1)]);
            __m256 u_derecha   = _mm256_loadu_ps(&u_antiguo[j * columnas_x + (i + 1)]);

            // suma_vecinos = u_arriba + u_abajo + u_izquierda + u_derecha
            __m256 suma_vecinos = _mm256_add_ps(_mm256_add_ps(u_arriba, u_abajo), _mm256_add_ps(u_izquierda, u_derecha));

            // esquema_5p = suma_vecinos - 4.0f * u_centro
            __m256 esquema_5p = _mm256_sub_ps(suma_vecinos, _mm256_mul_ps(vec_cuatro, u_centro));

            // u_nueva_vec = u_centro + vec_factor * esquema_5p
            __m256 u_nueva_vec = _mm256_add_ps(u_centro, _mm256_mul_ps(vec_factor, esquema_5p));

            _mm256_storeu_ps(&u_nuevo[j * columnas_x + i], u_nueva_vec);

            // Acumular diferencias al cuadrado
            __m256 diferencia = _mm256_sub_ps(u_nueva_vec, u_centro);
            suma_dif_al_cuadrado = _mm256_add_ps(suma_dif_al_cuadrado, _mm256_mul_ps(diferencia, diferencia));
        }

        // Suma horizontal de los residuos SIMD de la fila
        alignas(32) float temporal[8];
        _mm256_storeu_ps(temporal, suma_dif_al_cuadrado);
        double suma_fila = 0.0;
        for (int k = 0; k < 8; ++k)
        {
            suma_fila += temporal[k];
        }
        suma_diferencias += suma_fila;

        // Procesar elementos restantes secuencialmente
        for (; i < columnas_x - 1; ++i)
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

    // Calcular residuo norma L2
    residuo = std::sqrt(suma_diferencias / (columnas_x * filas_y));

    // 2. Mapear temperaturas a colores de píxeles
    for (uint32_t j = 0; j < filas_y; ++j)
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
