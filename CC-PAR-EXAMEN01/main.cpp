#include <fmt/core.h>
#include <omp.h>
#include <chrono>
#include "producto_escalar.h"

// Nombre: Edison Enriquez

#define ARRAY_SIZE 8000000
//#define ARRAY_SIZE 80000

int main() {
    int thread_count;
    #pragma omp parallel
    {
        #pragma omp master
        thread_count = omp_get_num_threads();
    }

    fmt::print("=================== Producto Escalar ===============================\n");
    fmt::print("Hilos de hardware detectados (OpenMP): {}\n", thread_count);
    fmt::print("Tamaño del arreglo de prueba: {} elementos\n\n", ARRAY_SIZE);

    float* X = new float[ARRAY_SIZE];
    float* Y = new float[ARRAY_SIZE];

    for (uint32_t i = 0; i < ARRAY_SIZE; ++i) {
        X[i] = 2.5f;
        Y[i] = 2.0f;
    }

    // 1: MODO SIMD 
    float res_simd = producto_escalar_simd(X, Y, ARRAY_SIZE);

    fmt::print("[1] MODO: SIMD (Unidad Vectorial)\n");
    fmt::print("    Resultado: {:.2f}\n\n", res_simd);

    // 2: MODO OPENMP REGIONES
    float res_omp = producto_escalar_regiones(X, Y, ARRAY_SIZE);

    fmt::print("[2] MODO: OpenMP Regiones Paralelas\n");
    fmt::print("    Resultado: {:.2f}\n\n", res_omp);

    delete[] X;
    delete[] Y;

    return 0;
}