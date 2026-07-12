#include <mpi.h>
#include <iostream>
#include <vector>
#include <fmt/core.h>
#include <cmath>
//multiplicar por dos un arreglo
#define N 17

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   
   int nprocs, rank;

   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N*1.0/nprocs);
   int padding = bloque * nprocs - N;

   int elementos = bloque;
   if (rank == nprocs - 1) {
      elementos = bloque - padding;
   }
   
   std::vector<double> local(bloque);
   int factor = 2;

   if (rank == 0) {
      std::vector<double> global(N, 0.0);
      for (int i = 0; i < N; i++) {
         global[i] = i;
      }
      
      for (int i = 1; i < nprocs; i++) {
         int cuantos = bloque;
         if (i == nprocs - 1) {
            cuantos = bloque - padding;
         }
         MPI_Send(&global[i*bloque], cuantos, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
      }
      for (int i = 0; i < bloque; i++) {
         local[i] = global[i];
      }
   } else {
      MPI_Recv(local.data(), elementos, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }

   for (int i = 0; i < elementos; i++) {
      local[i] = local[i] * 2.0;
   }
   
   if (rank == 0) {
      std::vector<double> res(N);
      for (int i = 0; i < bloque; i++) {
         res[i] = local[i];
      }
      
      for (int i = 1; i < nprocs; i++) {
         int cuantos = bloque;
         if (i == nprocs - 1) {
            cuantos = bloque - padding;
         }
         MPI_Recv(&res[i*bloque], cuantos, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      fmt::println("Resultado");
      for (int i = 0; i < N; i++) {
         fmt::print("{:.1f} ", res[i]);
      }
      
   } else {
      MPI_Send(local.data(), elementos, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
   }
   

   MPI_Finalize();
   return 0;
}
