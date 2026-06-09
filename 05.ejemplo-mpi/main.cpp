#include <iostream>
#include <fmt/core.h>
#include <mpi.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int version, subversion;
    MPI_Get_version(&version, &subversion);

    if (rank == 0) {
        fmt::println("MPI version: {}.{}", version, subversion);
        fmt::println("Number of processes: {}", nprocs);
    }

    fmt::println("RANK_{} de {} procesos", rank, nprocs);

    MPI_Finalize();

    //while()1 {
    //}

    // C:\oneAPI>setvars
    //C:\tools\prog-paralela\05.ejemplo-mpi\build\Debug>mpiexec -n 4 ejemplo-mpi.exe
    // https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html

    return 0;
}