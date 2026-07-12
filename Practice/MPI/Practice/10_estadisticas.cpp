#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <fmt/core.h>

#define DATOS 100

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   std::srand(std::time(nullptr) + rank);
   std::vector<double> mis_datos(DATOS);
   double mi_suma = 0.0;
   double mi_maximo = 1.0;
   double mi_minimo = 9999.0;

   for (int i = 0; i < DATOS; i++) {
      mis_datos[i] = (std::rand() % 10000) / 10.0 + 1;

      mi_suma += mis_datos[i];
      if (mis_datos[i] > mi_maximo) {
         mi_maximo = mis_datos[i];
      }
      if (mis_datos[i] < mi_minimo) {
         mi_minimo = mis_datos[i];
      }      
   }
   
   double suma_global, max_global, min_global;

   MPI_Reduce(&mi_suma, &suma_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   MPI_Reduce(&mi_maximo, &max_global, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
   MPI_Reduce(&mi_minimo, &min_global, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);

   if (rank == 0) {
      fmt::println("Datos Estadisticos");
      fmt::println("Datos totales analizados: {}", DATOS*nprocs);
      fmt::println("Promedio Global: {:.2f}", suma_global, (DATOS*nprocs));
      fmt::println("Minimo: {:.2f}", min_global);
      fmt::println("Maximo: {:.2f}", max_global);
   }
   
   MPI_Finalize();
   return 0;
}
