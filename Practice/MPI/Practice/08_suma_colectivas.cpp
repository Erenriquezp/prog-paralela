#include <mpi.h>
#include <fmt/core.h>
#include <vector>
#include <cmath>
#include <iostream>

#define N 15

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N*1.0/nprocs);
   int total_pad = bloque * nprocs;

   std::vector<double> A_local(bloque);
   std::vector<double> B_local(bloque);
   std::vector<double> C_local(bloque);

   std::vector<double> A;
   std::vector<double> B;
   std::vector<double> C;

   if (rank == 0) {
      A.resize(total_pad, 1.0);
      B.resize(total_pad, 2.0);
      C.resize(total_pad, 0.0);
   }

   MPI_Scatter(A.data(), bloque, MPI_DOUBLE, A_local.data(), bloque, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   MPI_Scatter(B.data(), bloque, MPI_DOUBLE, B_local.data(), bloque, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   
   for (int i = 0; i < bloque; i++) {
      C_local[i] = A_local[i] + B_local[i];
   }

   MPI_Gather(C_local.data(), bloque, MPI_DOUBLE, C.data(), bloque, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   
   if (rank == 0) {
      fmt::println("Resultado");
      for (int i = 0; i < N; i++) {
         fmt::print("{:.1f} ", C[i]);
      }  
   }
   
   MPI_Finalize();
   return 0;
}
