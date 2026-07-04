// ============================================================
// LISTADO 8.9: BARRERA PARA SINCRONIZAR TEMPORIZADORES
// MPI_Barrier detiene a todos hasta que TODOS llegan.
// Se pone una barrera ANTES de arrancar el reloj y otra ANTES
// de pararlo: así se mide el tiempo del proceso MÁS LENTO.
// MPI_Wtime() devuelve segundos (double).
// Ejecutar: mpiexec -n 4 03_barrera_wtime.exe
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

    MPI_Barrier(MPI_COMM_WORLD);            // esperar al más lento
    double main_time = MPI_Wtime() - start_time;

    if (rank == 0) {
        fmt::println("Tiempo del trabajo: {:.3f} segundos", main_time);
    }

    MPI_Finalize();
    return 0;
}
