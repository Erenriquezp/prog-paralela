// ============================================================
// LISTADO 8.15: GATHER PARA IMPRIMIR EN ORDEN
// La salida de varios ranks sale REVUELTA. Solución: cada rank
// manda su valor con MPI_Gather al rank 0, que los apila en un
// arreglo y los imprime EN ORDEN.
// Ejecutar: mpiexec -n 4 06_gather_impresion.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Trabajo simulado: cada rank mide un tiempo distinto (0.1 s * (rank+1))
    double start_time = MPI_Wtime();
    double t_fin = MPI_Wtime() + 0.1 * (rank + 1);
    while (MPI_Wtime() < t_fin) { }
    double total_time = MPI_Wtime() - start_time;

    // Cada rank aporta 1 valor; el rank 0 los recibe apilados en orden
    std::vector<double> times(nprocs);
    MPI_Gather(&total_time, 1, MPI_DOUBLE,      // lo que envía CADA rank
               times.data(), 1, MPI_DOUBLE,     // donde se apilan (solo rank 0)
               0, MPI_COMM_WORLD);

    // Solo imprime el rank 0: la salida sale ordenada
    if (rank == 0) {
        for (int i = 0; i < nprocs; i++) {
            fmt::println("{}: El trabajo tomo {:.3f} segundos", i, times[i]);
        }
    }

    MPI_Finalize();
    return 0;
}
