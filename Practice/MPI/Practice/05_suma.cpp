#include <vector>
#include <fmt/core.h>
#include <mpi.h>

#define N 12

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);

   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N * 1.0 / nprocs);
   int padding = bloque * nprocs - N;

   int elementos = bloque;
   if (rank == nprocs - 1)
   {
      elementos = bloque - padding;
   }

   std::vector<double> A_local(elementos);
   std::vector<double> B_local(elementos);
   std::vector<double> C_local(elementos);

   if (rank == 0)
   {
      std::vector<double> A_global(N, 1.0);
      std::vector<double> B_global(N, 1.0);

      for (int i = 1; i < nprocs; i++)
      {
         int cuantos = bloque;
         if (i == nprocs - 1)
         {
            cuantos = bloque - padding;
         }
         MPI_Send(&A_global[i * bloque], cuantos, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
         MPI_Send(&B_global[i * bloque], cuantos, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
      }
      for (int i = 0; i < bloque; i++)
      {
         A_local[i] = A_global[i];
         B_local[i] = B_global[i];
      }
   }
   else
   {
      MPI_Recv(A_local.data(), elementos, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(B_local.data(), elementos, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }

   for (int i = 0; i < elementos; i++)
   {
      C_local[i] = A_local[i] + B_local[i];
   }

   if (rank == 0)
   {
      std::vector<double> C_global(N);
      for (int i = 0; i < bloque; i++)
      {
         C_global[i] = C_local[i];
      }

      for (int i = 1; i < nprocs; i++)
      {
         int cuantos = bloque;
         if (i == nprocs - 1)
         {
            cuantos = bloque - padding;
         }
         MPI_Recv(&C_global[i * bloque], cuantos, MPI_DOUBLE, i, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      fmt::println("Resultado");
      for (int i = 0; i < N; i++)
      {
         fmt::print("{:.1f} ", C_global[i]);
      }
   }
   else
   {
      MPI_Send(C_local.data(), elementos, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);
   }

   MPI_Finalize();
   return 0;
}
