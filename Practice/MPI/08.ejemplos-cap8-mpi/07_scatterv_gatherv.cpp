// ============================================================
// LISTADO 8.16: SCATTERV / GATHERV (bloques de tamaño VARIABLE)
// Alternativa al padding: cada rank calcula su porción con la
// fórmula  inicio = N*rank/nprocs,  fin = N*(rank+1)/nprocs
// (reparte lo más parejo posible aunque N no sea divisible).
// Scatterv/Gatherv necesitan DOS arreglos: tamaños y offsets.
// Ejemplo: sumar 1.0 a cada elemento y verificar.
// Ejecutar: mpiexec -n 4 07_scatterv_gatherv.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <vector>

#define N 25    // NO divisible entre 4, a propósito

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Porción de cada rank con aritmética entera (sin padding)
    int inicio = N * rank / nprocs;
    int fin = N * (rank + 1) / nprocs;
    int nsize = fin - inicio;               // puede variar entre ranks

    // El maestro crea los datos globales
    std::vector<double> a_global;
    if (rank == 0) {
        a_global.resize(N);
        for (int i = 0; i < N; i++) {
            a_global[i] = i;
        }
    }

    // TODOS necesitan saber cuánto recibe cada rank y desde dónde
    std::vector<int> tamanios(nprocs), offsets(nprocs);
    MPI_Allgather(&nsize, 1, MPI_INT, tamanios.data(), 1, MPI_INT, MPI_COMM_WORLD);
    offsets[0] = 0;
    for (int i = 1; i < nprocs; i++) {
        offsets[i] = offsets[i - 1] + tamanios[i - 1];
    }

    // SCATTERV: reparte bloques de distinto tamaño según tamanios/offsets
    std::vector<double> a(nsize);
    MPI_Scatterv(a_global.data(), tamanios.data(), offsets.data(), MPI_DOUBLE,
                 a.data(), nsize, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // CÁLCULO LOCAL (aquí va la lógica del problema)
    for (int i = 0; i < nsize; i++) {
        a[i] = a[i] + 1.0;
    }

    // GATHERV: junta los bloques de distinto tamaño en el rank 0
    std::vector<double> a_test;
    if (rank == 0) {
        a_test.resize(N);
    }
    MPI_Gatherv(a.data(), nsize, MPI_DOUBLE,
                a_test.data(), tamanios.data(), offsets.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    // Verificación en el maestro
    if (rank == 0) {
        int errores = 0;
        for (int i = 0; i < N; i++) {
            if (a_test[i] != a_global[i] + 1.0) {
                errores++;
            }
        }
        fmt::println("Reporte: {} correctos, {} errores", N - errores, errores);
    }

    MPI_Finalize();
    return 0;
}
