#include "fractal_simd.h"
#include "palette.h"

#include <cstring>
#include <complex>

#include <immintrin.h> // Para intrínsecos SIMD (AVX2), vamos a utilizar 8 valores float

extern int max_iteraciones;
extern std::complex<double> c;

void julia_simd(double x_min, double y_min, double x_max, double y_max, uint32_t width, uint32_t height, uint32_t* pixel_buffer) {
    // Implementación SIMD aquí
    std::memset(pixel_buffer, 0xFF000000, width * height * sizeof(uint32_t));

    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    // (xmin, xmin, xmin, xmin, ymin, ymin, ymin, ymin) para cargar en registros SIMD, 8 veces
    // (-1.5, -1.5, -1.5, -1.5, -1.0, -1.0, -1.0, -1.0) para cargar en registros SIMD, 8 veces
    __m256 xmin = _mm256_set1_ps(x_min);

    // (ymax, ymax, ymax, ymax, ymax, ymax, ymax, ymax) para cargar en registros SIMD, 8 veces
    // (1, 1, 1, 1, 1, 1, 1, 1) para cargar en registros SIMD, 8 veces
    __m256 ymax = _mm256_set1_ps(y_max);

    __m256 xscale = _mm256_set1_ps(dx); // (dx, dx, dx, dx, dx, dx, dx, dx)
    __m256 yscale = _mm256_set1_ps(dy); // (dy, dy, dy, dy, dy, dy, dy, dy)

    __m256 c_real = _mm256_set1_ps(c.real()); // (creal, creal, ..., creal)
    __m256 c_imag = _mm256_set1_ps(c.imag()); // (cimag, cimag, ..., cimag)

    __m256 max_norma = _mm256_set1_ps(4.0f); // (4, 4, 4, 4, 4, 4, 4, 4) para comparar con la norma de los complejos
    __m256 one = _mm256_set1_ps(1.0f); // (1, 1, 1, 1, 1, 1, 1, 1) para incrementar el contador de iteraciones
    
    for (int i=0; i<width; i++) {
        for (int j=0; j<height; j+=8) { // Procesamos 8 píxeles a la vez
            __m256 mx = _mm256_set1_ps(i); // (i, i, i, i, i, i, i, i)
            __m256 my = _mm256_set_ps(j+7, j+6, j+5, j+4, j+3, j+2, j+1, j+0); // (j+7, j+6, ..., j)

            // xmin+mx*xscale -> (x0, x1, x2, x3, x4, x5, x6, x7) -- real
            __m256 cr = _mm256_add_ps(xmin, _mm256_mul_ps(mx, xscale));

            // ymax-my*yscale -> (y0, y1, y2, y3, y4, y5, y6, y7) -- imaginario
            __m256 ci = _mm256_sub_ps(ymax, _mm256_mul_ps(my, yscale));

            //verificar si los 8 complejos están acotados o no
            int iter = 1;
            __m256 mk = _mm256_set1_ps(iter);
            
            __m256 zr = cr; // z0 = creal
            __m256 zi = ci; // z0 = cimag

            while (iter<max_iteraciones){
                // Zn+1 = Zn^2 + C
                __m256 zr2 = _mm256_mul_ps(zr, zr); // zr^2
                __m256 zi2 = _mm256_mul_ps(zi, zi); // zi^2
                __m256 zrzi = _mm256_mul_ps(zr, zi); // zr*zi

                zr = _mm256_add_ps(_mm256_sub_ps(zr2, zi2), c_real); // zr^2 - zi^2 + creal
                zi = _mm256_add_ps(_mm256_add_ps(zrzi, zrzi), c_imag); // 2*zr*zi + cimag

                // Calcular la norma de los complejos: |Z|^2 = zr^2 + zi^2
                zr2 = _mm256_mul_ps(zr, zr); // zr^2
                zi2 = _mm256_mul_ps(zi, zi); // zi^2
                __m256 norma2 = _mm256_add_ps(zr2, zi2); // |Z|^2

                // Crear una máscara para los complejos que aún no han escapado (norma2 < 4)
                __m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ); // (norma2 <= 4) -> 0xFFFFFFFF, (norma2 > 4) -> 0x00000000

                // Incrementar el contador de iteraciones solo para los complejos que aún no han escapado
                mk = _mm256_add_ps(mk, _mm256_and_ps(mask, one)); // mk += (mask & 1)

                if (_mm256_testz_ps(mask, _mm256_set1_ps(-1))) { // Si todos los complejos han escapado, salimos del bucle
                    break;
                }

                iter++;
            }

            float d[8];
            _mm256_storeu_ps(d, mk); // Almacenar los contadores de iteraciones en un arreglo para asignar colores
            
            for (int it=0; it<8; it++) {
                int index = (j+it)*width + i; // Índice del píxel en el buffer

                if (index < width*height) { // Verificar que no se salga del buffer
                    if (d[it] < max_iteraciones) {
                        int color_index = (int)(d[it]) % PALETTE_SIZE; // Obtener el índice del color en la paleta
                        pixel_buffer[index] = color_ramp[color_index]; // Asignar el color al píxel
                    } else {
                        pixel_buffer[index] = 0xFF000000; // Negro para los puntos que no escapan
                    }
                }
            } 
        }
    }
}