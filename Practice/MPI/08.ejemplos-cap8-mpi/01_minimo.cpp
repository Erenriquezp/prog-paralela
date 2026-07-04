// ============================================================
// LISTADO 8.1: EJEMPLO MÍNIMO FUNCIONAL DE MPI
// Todo programa MPI arranca con Init y termina con Finalize.
// La salida puede aparecer en CUALQUIER orden.
// Ejecutar: mpiexec -n 4 01_minimo.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    fmt::println("Rank {} of {}", rank, nprocs);

    MPI_Finalize();
    return 0;
}
