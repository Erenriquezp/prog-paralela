#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath>
// Ejemplo: multiplicar por un factor cada elemento y sumar todo.

#define N 25

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int nprocs, rank;
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   int bloque = std::ceil(N*1.0/nprocs);
   int total_pad = bloque * nprocs;

   int factor = 0;
   std::vector<double> local(bloque);

   std::vector<double> datos;
   std::vector<double> resultado;

   if (rank == 0) {
      datos.resize(total_pad, 0.0);
      resultado.resize(total_pad, 0.0);
      for (int i = 0; i < N; i++) {
         datos[i] = i;
      }
      factor = 3;
   }

   MPI_Bcast(&factor, 1, MPI_INT, 0, MPI_COMM_WORLD);

   MPI_Scatter(datos.data(), bloque, MPI_DOUBLE,
               local.data(), bloque, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

   double suma_local = 0.0;
   for (int i = 0; i < bloque; i++) {
      local[i] = local[i] * factor;
      suma_local = suma_local + local[i];
   }
   
   MPI_Gather(local.data(), bloque, MPI_DOUBLE,
               resultado.data(), bloque, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

   double suma_total = 0.0;
   MPI_Reduce(&suma_local, &suma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

   if (rank == 0) {
      fmt::println("Resultado");
      for (int i = 0; i < N; i++) {
         fmt::print("{:.1f} ", resultado[i]);
      }
      fmt::println("");
      fmt::println("Suma: {:.1f}", suma_total);
   }
   
   MPI_Finalize();
   return 0;
}
