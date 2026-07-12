#include <fmt/core.h>
#include <fstream>
#include <mpi.h>
#include <string>
#include <cmath>

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
      std::string line;
      while (std::getline(archivo, line)) {
         contenido = contenido + line + "\n";
      }
      tam = contenido.size();
   }

   MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);
   
   int bloque = std::ceil(tam*1.0/nprocs);
   int total_pad = bloque * nprocs;

   if (rank == 0) {
      contenido.resize(total_pad, ' ');
   }

   std::string contenido_local(bloque, ' ');

   MPI_Scatter(contenido.data(), bloque, MPI_CHAR, 
               contenido_local.data(), bloque, MPI_CHAR, 0, MPI_COMM_WORLD);

   int vocales = 0;

   for (int i = 0; i < bloque; i++) {
      char c = contenido_local[i];
      if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
         c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') {
         vocales++;
      }
   }
   int total_vocales = 0;
   MPI_Reduce(&vocales, &total_vocales, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
   
   if (rank == 0) {
      fmt::println("Archivo procesado tam:{}", tam);
      fmt::println("Vocales totales: {}", total_vocales);
   }
      
   MPI_Finalize();
   return 0;
}
