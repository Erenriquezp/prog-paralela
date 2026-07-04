// ============================================================
// PLANTILLA 3: COMUNICACIÓN COLECTIVA (Bcast, Scatter, Gather, Reduce)
// Mismo problema que la plantilla 2, pero SIN Send/Recv manuales.
// Truco del PADDING: se agranda el arreglo global hasta un múltiplo
// de nprocs (relleno con 0) para que Scatter/Gather repartan igual.
// Las colectivas las ejecutan TODOS los procesos, SIN 'if (rank...)'.
// Ejemplo: multiplicar por un factor cada elemento y sumar todo.
// Ejecutar: mpiexec -n 4 03_colectivas.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath>

#define N 25    // tamaño real del problema

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Reparto uniforme: TODOS calculan las mismas cuentas
    int bloque = std::ceil(N * 1.0 / nprocs);   // elementos por proceso
    int n_pad = bloque * nprocs;                // tamaño agrandado (padding)

    int factor = 0;                             // dato que se difunde a todos
    std::vector<double> local(bloque);          // bloque de trabajo de cada rank

    // Arreglos globales: SOLO el maestro los llena (en los demás quedan vacíos)
    std::vector<double> datos;
    std::vector<double> resultado;

    // --- FASE 1: INICIALIZACIÓN (solo el maestro) ---
    if (rank == 0) {
        datos.resize(n_pad, 0.0);               // relleno con 0 = padding
        resultado.resize(n_pad, 0.0);
        for (int i = 0; i < N; i++) {
            datos[i] = i;
        }
        factor = 2;
    }

    // --- FASE 2: DISTRIBUCIÓN (colectivas, las ejecutan TODOS) ---

    // BCAST: el rank 0 envía el MISMO dato a todos
    MPI_Bcast(&factor, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // SCATTER: el rank 0 parte 'datos' y entrega un bloque a cada uno
    MPI_Scatter(datos.data(), bloque, MPI_DOUBLE,   // origen (solo cuenta en rank 0)
                local.data(), bloque, MPI_DOUBLE,   // destino (en todos)
                0, MPI_COMM_WORLD);                 // 0 = quien reparte

    // --- FASE 3: CÁLCULO LOCAL (aquí va la lógica del problema) ---
    double suma_local = 0.0;
    for (int i = 0; i < bloque; i++) {
        local[i] = local[i] * factor;           // el padding vale 0, no molesta
        suma_local = suma_local + local[i];
    }

    // --- FASE 4: RECOLECCIÓN (colectivas, las ejecutan TODOS) ---

    // GATHER: cada uno envía su bloque y el rank 0 los pega en orden
    MPI_Gather(local.data(), bloque, MPI_DOUBLE,        // origen (en todos)
               resultado.data(), bloque, MPI_DOUBLE,    // destino (solo cuenta en rank 0)
               0, MPI_COMM_WORLD);                      // 0 = quien recolecta

    // REDUCE: combina los valores de todos en uno solo (suma, máximo, etc.)
    double suma_total = 0.0;
    MPI_Reduce(&suma_local, &suma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // --- FASE 5: IMPRESIÓN (solo el maestro) ---
    if (rank == 0) {
        fmt::println("========== RESULTADO (RANK_0) ==========");
        for (int i = 0; i < N; i++) {           // solo hasta N: se ignora el padding
            fmt::print("{:.1f} ", resultado[i]);
        }
        fmt::println("");
        fmt::println("Suma total (Reduce): {:.1f}", suma_total);
    }

    MPI_Finalize();
    return 0;
}
