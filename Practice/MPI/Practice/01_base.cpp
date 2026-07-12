#include <iostream>
#include <vector>
#include <mpi.h>
#include <cmath>
#include <fmt/core.h>

#define N 15

void imprimir(std::vector<double> vector, int n) {
   for (int i = 0; i < n; i++) {
      fmt::print("{:.1f} ", vector[i]);
   }
   fmt::println("");
}

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);

   int rank, nprocs;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N*1.0 / nprocs);
   int total_padd = bloque * nprocs;

   std::vector<double> global_v(total_padd, 0.0);
   std::vector<double> local_v(bloque);

   if (rank == 0) {
      for (int i = 0; i < N; i++) {
         global_v[i] = i + 1;
      }
      for (int i = 1; i < nprocs; i++) {
         MPI_Send(&global_v[i*bloque], bloque, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
      }
      for (int i = 0; i < bloque; i++) {
         local_v[i] = global_v[i];
      }
      imprimir(global_v, N);
   } else {
      MPI_Recv(local_v.data(), bloque, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }
   
   for (int i = 0; i < bloque; i++) {
      local_v[i] = local_v[i] * 2;
   }

   if (rank == 0) {
      std::vector<double> resultado(N);

      for (int i = 0; i < bloque; i++) {
         resultado[i] = local_v[i];
      }

      for (int i = 1; i < nprocs; i++) {
         MPI_Recv(&resultado[i*bloque], bloque, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      imprimir(resultado, N);      
   } else {
      MPI_Send(local_v.data(), bloque, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
   }

   MPI_Finalize();
   return 0;
}
