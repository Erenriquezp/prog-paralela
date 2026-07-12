#include <fmt/core.h>
#include <mpi.h>
#include <fstream>
#include <string>

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   std::string contenido;
   int tam = 0;

   if (rank == 0) {
      std::ifstream archivo("entrada.txt");
      std::string linea;

      while (std::getline(archivo, linea)) {
         contenido = contenido + linea + "\n";
      }
      tam = contenido.size();      
   }

   MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);
   contenido.resize(tam);

   MPI_Bcast(contenido.data(), tam, MPI_CHAR, 0, MPI_COMM_WORLD);

   fmt::print("RANK_{} leyo {} caracteres:\n{}", rank, tam, contenido);
   
   MPI_Finalize();
   return 0;
}
