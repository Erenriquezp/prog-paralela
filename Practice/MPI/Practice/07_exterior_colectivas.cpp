#include <mpi.h>
#include <fmt/core.h>
#include <vector>
#include <cmath>
#include <iostream>

#define N 9

void imprimir_matriz(std::vector<double> matriz, int n) {
   for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
         fmt::print("{:.1f} ", matriz[i*n+j]);
      }
      fmt::println("");
   }
}

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N*1.0/nprocs);
   int total_pad = bloque * nprocs;

   std::vector<double> A(N);
   std::vector<double> B_local(bloque);
   std::vector<double> C_local(bloque * N);

   std::vector<double> B;
   std::vector<double> C;

   if (rank == 0) {
      B.resize(N, 0.0);
      C.resize(N*total_pad);

      for (int i = 0; i < N; i++) {
         A[i] = i+1;
         B[i] = i+1;
      }
   }
   
   MPI_Bcast(A.data(), N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

   MPI_Scatter(B.data(), bloque, MPI_DOUBLE,
               B_local.data(), bloque, MPI_DOUBLE, 0, MPI_COMM_WORLD);

   for (int i = 0; i < bloque; i++) {
      for (int j = 0; j < N; j++) {
         C_local[i*N+j] = B_local[i] * A[j];
      }
   }

   MPI_Gather(C_local.data(), bloque * N, MPI_DOUBLE,
               C.data(), bloque * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   
   if (rank == 0) {
      fmt::println("Respuesta");
      imprimir_matriz(C, N);
   }
   
   MPI_Finalize();
   return 0;
}
