// ============================================================
// PLANTILLA 2: COMUNICACIÓN PUNTO A PUNTO (MPI_Send / MPI_Recv)
// Patrón maestro-esclavo en 3 fases:
//   FASE 1: el maestro reparte los datos (Send / Recv)
//   FASE 2: todos calculan su parte
//   FASE 3: el maestro recolecta los resultados (Recv / Send)
// Ejemplo: multiplicar por 2 cada elemento de un arreglo.
// Ejecutar: mpiexec -n 4 02_punto_a_punto.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath>

#define N 25    // tamaño del problema (NO divisible entre 4, a propósito)

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Reparto: TODOS calculan las mismas cuentas
    int bloque = std::ceil(N * 1.0 / nprocs);   // elementos por proceso
    int padding = bloque * nprocs - N;          // lo que sobra al final

    int mis_elementos = bloque;
    if (rank == nprocs - 1) {
        mis_elementos = bloque - padding;       // el ÚLTIMO rank recibe menos
    }

    std::vector<double> local(bloque);          // bloque de trabajo de cada rank

    // --- FASE 1: DISTRIBUCIÓN ---
    if (rank == 0) {
        // El maestro crea TODOS los datos
        std::vector<double> datos(N);
        for (int i = 0; i < N; i++) {
            datos[i] = i;
        }

        // Envía un bloque a cada esclavo (tag 0)
        for (int i = 1; i < nprocs; i++) {
            int cuantos = bloque;
            if (i == nprocs - 1) {
                cuantos = bloque - padding;
            }
            MPI_Send(&datos[i * bloque], cuantos, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }

        // El maestro se queda con el primer bloque
        for (int i = 0; i < bloque; i++) {
            local[i] = datos[i];
        }
    } else {
        // Cada esclavo recibe su bloque (tag 0)
        MPI_Recv(local.data(), mis_elementos, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // --- FASE 2: CÁLCULO LOCAL (aquí va la lógica del problema) ---
    for (int i = 0; i < mis_elementos; i++) {
        local[i] = local[i] * 2.0;
    }

    // --- FASE 3: RECOLECCIÓN ---
    if (rank == 0) {
        std::vector<double> resultado(N);

        // El maestro copia su propio bloque
        for (int i = 0; i < bloque; i++) {
            resultado[i] = local[i];
        }

        // Y recibe el bloque de cada esclavo (tag 1 para no chocar con la fase 1)
        for (int i = 1; i < nprocs; i++) {
            int cuantos = bloque;
            if (i == nprocs - 1) {
                cuantos = bloque - padding;
            }
            MPI_Recv(&resultado[i * bloque], cuantos, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        fmt::println("========== RESULTADO (RANK_0) ==========");
        for (int i = 0; i < N; i++) {
            fmt::print("{:.1f} ", resultado[i]);
        }
        fmt::println("");
    } else {
        // Cada esclavo devuelve su bloque calculado (tag 1)
        MPI_Send(local.data(), mis_elementos, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
