#include <mpi.h>
#include <fmt/core.h>
#include <cmath>
#include <string>
#include <fstream>

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   int tam = 0;
   std::string contenido;

   if (rank == 0)
   {
      std::string line;
      std::fstream archivo("entrada.txt");
      while (std::getline(archivo, line))
      {
         contenido = contenido + line + "\n";
      }
      tam = contenido.size();
   }

   MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);

   int bloque = std::ceil(tam * 1.0 / nprocs);
   int total_pad = bloque * nprocs;

   if (rank == 0)
   {
      contenido.resize(total_pad, ' ');
   }

   std::string contenido_local(bloque, ' ');
   MPI_Scatter(contenido.data(), bloque, MPI_CHAR, contenido_local.data(), bloque, MPI_CHAR, 0, MPI_COMM_WORLD);

   int vocales = 0;
   for (int i = 0; i < bloque; i++)
   {
      char c = contenido_local[i];
      if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
          c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U')
      {
         vocales++;
      }
   }

   int total = 0;
   MPI_Reduce(&vocales, &total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

   if (rank == 0)
   {
      fmt::println("Resultado");
      fmt::println("Total de vocales: {}", total);
   }

   MPI_Finalize();
   return 0;
}
