#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <fmt/core.h>

#define N 12
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int bloque = std::ceil(N * 1.0 / nprocs);
    int total_pad = bloque * nprocs;

    std::vector<double> local_A(bloque * N, 0.0);
    std::vector<double> x(N, 0.0); 
    std::vector<double> local_y(bloque, 0.0);

    std::vector<double> A_global;
    std::vector<double> y_global;

    if (rank == 0) {
        A_global.resize(total_pad * N, 0.0);
        y_global.resize(total_pad, 0.0);

        for (int i = 0; i < N; i++) {
            x[i] = 2.0;
            for (int j = 0; j < N; j++) {
                A_global[i * N + j] = 1.0;
            }
        }
    }

    if (rank == 0) {
        for (int i = 1; i < nprocs; i++) {
            MPI_Send(x.data(), N, MPI_DOUBLE, i, 10, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(x.data(), N, MPI_DOUBLE, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank == 0) {
        for (int i = 1; i < nprocs; i++) {
            MPI_Send(&A_global[i * bloque * N], bloque * N, MPI_DOUBLE, i, 20, MPI_COMM_WORLD);
        }
        for (int i = 0; i < bloque * N; i++) {
            local_A[i] = A_global[i];
        }
    } else {
        MPI_Recv(local_A.data(), bloque * N, MPI_DOUBLE, 0, 20, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < bloque; i++) {
        double suma = 0.0;
        for (int j = 0; j < N; j++) {
            suma += local_A[i * N + j] * x[j];
        }
        local_y[i] = suma;
    }

    if (rank == 0) {
        for (int i = 0; i < bloque; i++) {
            y_global[i] = local_y[i];
        }
        for (int i = 1; i < nprocs; i++) {
            MPI_Recv(&y_global[i * bloque], bloque, MPI_DOUBLE, i, 30, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Send(local_y.data(), bloque, MPI_DOUBLE, 0, 30, MPI_COMM_WORLD);
    }

    if (rank == 0) {
        fmt::print("=== RESULTADO DEL VECTOR Y (y = A * x) ===\n");
        for (int i = 0; i < N; i++) {
            fmt::print("y[{}] = {:.1f}\n", i, y_global[i]);
        }
    }

    MPI_Finalize();
    return 0;
}