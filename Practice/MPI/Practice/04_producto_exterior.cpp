#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <fmt/core.h>

#define N 9

// Función para imprimir una matriz
void imprimir_matriz(const std::vector<double>& M, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fmt::print("{:6.1f} ", M[i * n + j]);
        }
        fmt::print("\n");
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // 1. MATEMÁTICA: Bloques iguales (con padding)
    int bloque = std::ceil(N * 1.0 / nprocs);
    int total_pad = bloque * nprocs;

    std::vector<double> V(N);
    std::vector<double> local_U(bloque);
    std::vector<double> local_res(bloque * N);

    // --- FASE 1: DISTRIBUCIÓN ---
    if (rank == 0) {
        std::vector<double> U(total_pad, 0.0);
        for(int i = 0; i < N; i++) { 
            U[i] = i+1; 
            V[i] = i+1; 
         }

        for (int i = 1; i < nprocs; i++) {
            MPI_Send(V.data(), N, MPI_DOUBLE, i, 0, MPI_COMM_WORLD); // Tag 0: V
            MPI_Send(&U[i * bloque], bloque, MPI_DOUBLE, i, 1, MPI_COMM_WORLD); // Tag 1: U
        }
        for (int i = 0; i < bloque; i++) {
            local_U[i] = U[i];
        }
        
    } else {
        MPI_Recv(V.data(), N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_U.data(), bloque, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // --- FASE 2: CÁLCULO ---
    for (int i = 0; i < bloque; i++) {
        for (int j = 0; j < N; j++) {
            local_res[i * N + j] = local_U[i] * V[j];
        }
    }

    // --- FASE 3: RECOLECCIÓN Y RESULTADO ---
    if (rank == 0) {
        std::vector<double> global_res(total_pad * N);
        for (int i = 0; i < local_res.size(); i++) {
            global_res[i] = local_res[i];
        }
                
        for (int i = 1; i < nprocs; i++) {
            // TAG 2: Recolectar partes de la matriz
            MPI_Recv(&global_res[i * bloque * N], bloque * N, MPI_DOUBLE, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        fmt::print("Matriz resultante:\n");
        imprimir_matriz(global_res, N); // Imprimimos solo hasta NxN
        
    } else {
        MPI_Send(local_res.data(), bloque * N, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD); // Tag 2
    }

    MPI_Finalize();
    return 0;
}