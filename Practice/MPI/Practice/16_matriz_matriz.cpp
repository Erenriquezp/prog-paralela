#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <fmt/core.h>

#define N 8 

void imprimir_matriz(const std::vector<double>& M, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            fmt::print("{:6.1f} ", M[i * columnas + j]);
        }
        fmt::print("\n");
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int bloque = std::ceil(N * 1.0 / nprocs);
    int total_pad = bloque * nprocs;

    std::vector<double> local_A(bloque * N, 0.0);
    std::vector<double> local_C(bloque * N, 0.0);
    std::vector<double> B(N * N, 0.0);

    std::vector<double> A_global;
    std::vector<double> C_global;

    if (rank == 0) {
        A_global.resize(total_pad * N, 0.0);
        C_global.resize(total_pad * N, 0.0);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A_global[i * N + j] = i + 1.0;
                B[i * N + j] = (i == j) ? 1.0 : 0.0;
            }
        }

        fmt::print("=== MATRIZ A (ORIGINAL) ===\n");
        imprimir_matriz(A_global, N, N);
        fmt::print("\n=== MATRIZ B (IDENTIDAD) ===\n");
        imprimir_matriz(B, N, N);
        fmt::print("\n");
    }

    MPI_Bcast(B.data(), N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  
    MPI_Scatter(A_global.data(), bloque * N, MPI_DOUBLE,
                local_A.data(), bloque * N, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    for (int i = 0; i < bloque; i++) {
        for (int j = 0; j < N; j++) {
            double suma = 0.0;
            for (int k = 0; k < N; k++) {
                suma += local_A[i * N + k] * B[k * N + j];
            }
            local_C[i * N + j] = suma;
        }
    }

    MPI_Gather(local_C.data(), bloque * N, MPI_DOUBLE,
               C_global.data(), bloque * N, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        fmt::print("=== MATRIZ RESULTANTE C (C = A * B) ===\n");
        imprimir_matriz(C_global, N, N); 
    }

    MPI_Finalize();
    return 0;
}