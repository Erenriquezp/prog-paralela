#include <fmt/core.h>
#include <mpi.h>

int main(int argc, char** argv)
{
   MPI_Init(&argc, &argv);

   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (rank == 0) {
      fmt::println("Soy el MAESTRO, hay {} procesos", nprocs);
   }
   
   fmt::println("Hola desde RANK_{}, de {}", rank, nprocs);

   MPI_Finalize();

   return 0;
}
