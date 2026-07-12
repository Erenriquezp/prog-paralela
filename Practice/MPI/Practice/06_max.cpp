#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath>
#include <iostream>

#define N 16

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

   std::vector<double> local(elementos);

   if(rank == 0) {
      std::vector<double> global = {
         1, 2, 2, 4, 3, 45, 2, 3, 0, 5, 3, 7, 8, 6, 5, 9
      };
      for (int i = 0; i < elementos; i++) {
         local[i] = global[i];
      }
      for (int i = 1; i < nprocs; i++) {
         int cuantos = bloque;
         if (i == nprocs - 1) {
            cuantos = bloque - padding;
         }
         MPI_Send(&global[i*bloque], cuantos, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);         
      }     
   } else {
      MPI_Recv(local.data(), elementos, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }

   double max_local = 0.0;
   for (int i = 0; i < elementos; i++) {
      if (max_local < local[i]) {
         max_local = local[i];
      }
   }

   if (rank == 0) {
      double max_global = 0.0;
      std::vector<double> maxs(nprocs-1);
      if (max_global < max_local) {
         max_global = max_local;
      }

      for (int i = 1; i < nprocs; i++) {
         MPI_Recv(&maxs[i-1], 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }

      for (int i = 0; i < nprocs-2; i++) {
         if (max_global < maxs[i]){
            max_global = maxs[i];
         }
      }
      fmt::println("Maximo: {}", max_global);
   
   } else {
      MPI_Send(&max_local, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
   }
   
   MPI_Finalize();
   return 0;
}
