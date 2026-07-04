// ============================================================
// LISTADO 8.11: REDUCE PARA OBTENER MIN, MAX Y PROMEDIO
// Cada rank mide su tiempo y MPI_Reduce los combina en UN valor
// que queda SOLO en el rank 0 (con Allreduce quedaría en todos).
// Operaciones: MPI_MAX, MPI_MIN, MPI_SUM, MPI_MINLOC, MPI_MAXLOC.
// Ejecutar: mpiexec -n 4 05_reduce_min_max_avg.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    MPI_Barrier(MPI_COMM_WORLD);            // todos arrancan a la vez
    double start_time = MPI_Wtime();

    // Trabajo simulado: cada rank tarda distinto (0.1 s * (rank+1))
    double t_fin = MPI_Wtime() + 0.1 * (rank + 1);
    while (MPI_Wtime() < t_fin) { }

    double main_time = MPI_Wtime() - start_time;

    // Combinar el tiempo de todos los ranks en un solo valor (en rank 0)
    double min_time, max_time, avg_time;
    MPI_Reduce(&main_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&main_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&main_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        fmt::println("Tiempos -> Min: {:.3f}  Max: {:.3f}  Avg: {:.3f} segundos",
                     min_time, max_time, avg_time / nprocs);
    }

    MPI_Finalize();
    return 0;
}
