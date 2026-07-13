#include <mpi.h>
#include <string>
#include <fmt/core.h>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   std::string contenido;
   int tam = 0;

   if (rank == 0) {
      std::fstream archivo("entrada.txt");
      std::string line;
      while (std::getline(archivo, line)) {
         contenido = contenido + line + "\n";
      }
      tam = contenido.size();
   }
   
   if (rank == 0) {
      for (int i = 1; i < nprocs; i++) {
         MPI_Send(&tam, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }       
   } else {
      MPI_Recv(&tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }
   
   int bloque = std::ceil(tam*1.0/nprocs);
   int total_pad = bloque * nprocs;

   if (rank == 0) {
      contenido.resize(total_pad, ' ');
   }
   
   std::string contenido_local(bloque, ' ');

   if (rank == 0) {
      for (int i = 1; i < nprocs; i++) {
         MPI_Send(&contenido[i*bloque], bloque, MPI_CHAR, i, 1, MPI_COMM_WORLD);
      }
      for (int i = 0; i < bloque; i++) {
         contenido_local[i] = contenido[i];
      }      
   } else {
      MPI_Recv(contenido_local.data(), bloque, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }
   
   int vocales = 0;
   for (int i = 0; i < bloque; i++) {
      char c = contenido_local[i];
       if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
            c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') {
            vocales++;
        }      
   }
   
   int vocales_totales = 0;

   if (rank == 0) {
      vocales_totales = vocales;
      for (int i = 1; i < nprocs; i++) {
         int aux = 0;
         MPI_Recv(&aux, 1, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         vocales_totales += aux;
      }
      fmt::println("Resultado");
      fmt::println("Total de vocales: {}", vocales_totales);
   } else {
      MPI_Send(&vocales, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
   }
   
   MPI_Finalize();
   return 0;
}
