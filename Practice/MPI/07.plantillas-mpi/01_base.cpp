// ============================================================
// PLANTILLA 1: ESQUELETO BASE DE TODO PROGRAMA MPI
// Ejecutar: mpiexec -n 4 01_base.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);                     // 1. Arrancar MPI (SIEMPRE primero)

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);     // 2. ¿Cuántos procesos somos?
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);       // 3. ¿Quién soy yo? (0..nprocs-1)

    if (rank == 0) {
        fmt::println("Soy el MAESTRO, hay {} procesos", nprocs);
    }

    fmt::println("Hola desde RANK_{} de {}", rank, nprocs);

    MPI_Finalize();                             // 4. Cerrar MPI (SIEMPRE al final)
    return 0;
}
