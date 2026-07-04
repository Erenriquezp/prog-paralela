// ============================================================
// LISTADO 8.17: STREAM TRIAD CON MPI (ancho de banda de memoria)
// Paralelismo de datos SIN comunicación: cada rank trabaja sobre
// su porción del arreglo (misma fórmula inicio/fin del listado 8.16)
// y no necesita hablar con nadie. Mide c[i] = a[i] + escalar*b[i].
// Ejecutar: mpiexec -n 4 08_stream_triad.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <vector>

#define NTIMES 16
#define STREAM_ARRAY_SIZE 20000000

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Porción de cada rank (reparto sin padding)
    long inicio = (long)STREAM_ARRAY_SIZE * rank / nprocs;
    long fin = (long)STREAM_ARRAY_SIZE * (rank + 1) / nprocs;
    int nsize = fin - inicio;

    std::vector<double> a(nsize, 1.0);
    std::vector<double> b(nsize, 2.0);
    std::vector<double> c(nsize);

    double escalar = 3.0;
    double time_sum = 0.0;

    for (int k = 0; k < NTIMES; k++) {
        double t_start = MPI_Wtime();
        for (int i = 0; i < nsize; i++) {
            c[i] = a[i] + escalar * b[i];
        }
        time_sum = time_sum + (MPI_Wtime() - t_start);
        c[1] = c[2];    // evita que el compilador elimine el bucle
    }

    if (rank == 0) {
        fmt::println("Tiempo promedio del triad: {:.3f} ms", time_sum / NTIMES * 1000.0);
    }

    MPI_Finalize();
    return 0;
}
