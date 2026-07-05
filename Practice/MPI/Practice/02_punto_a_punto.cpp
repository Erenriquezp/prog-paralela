#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath>

#define N 25

int main(int argc, char** argv)
{
   MPI_Init(&argc, &argv);

   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N*1.0 / nprocs);
   int padding = bloque * nprocs - N;

   int mis_elementos = bloque;
   if (rank == nprocs - 1) {
      mis_elementos = bloque - padding;
   }

   std::vector<double> local(bloque);

   if (rank == 0) {
      std::vector<double> datos(N);
      for (int i = 0; i < N; i++) {
         datos[i] = i;
      }

      for (int i = 1; i < nprocs; i++) {
         int cuantos = bloque;
         if (i == nprocs - 1) {
            cuantos = bloque - padding;
         }
         MPI_Send(&datos[i * bloque], cuantos, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);         
      }

      for (int i = 0; i < bloque; i++) {
         local[i] = datos[i];
      }   

   } else {
      MPI_Recv(local.data(), mis_elementos, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }
   
   for (int i = 0; i < mis_elementos; i++) {
      local[i] = local[i] * 2.0;
   }

   if (rank == 0) {
      std::vector<double> resultado(N);

      for (int i = 0; i < bloque; i++) {
         resultado[i] = local[i];
      }

      for (int i = 1; i < nprocs; i++) {
         int cuantos = bloque;
         if (i == nprocs - 1) {
            cuantos = bloque - padding;
         }
         MPI_Recv(&resultado[i * bloque], cuantos, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      
      fmt::println("============Resultado (RANK_0)=================");
      for (int i = 0; i < N; i++) {
         fmt::print("{:.1f} ", resultado[i]);
      }
      fmt::println("");
   } else {
      MPI_Send(local.data(), mis_elementos, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
   }
    
   MPI_Finalize();

   return 0;
}
