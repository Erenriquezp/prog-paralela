#include "heat_mpi.h"

#include <cmath>
#include <vector>
#include <fmt/core.h>
#include <mpi.h>

// ============================================================
// TODO (Integrante D) — sección 6 del PLAN.md completa.
// Referencias de patrón:
//   - 06.fractal-mpi/main.cpp        (maestro/trabajador, Bcast de control)
//   - Practice/MPI/Practice/*.cpp    (padding, Scatter/Gather/Allreduce)
//
// Sugerencia de funciones internas (static) para mantener run_mpi corto:
//
//   static void intercambiar_fantasmas(std::vector<double>& u_local, ...)
//     Punto a punto con los vecinos rank-1 y rank+1 (sección 6.2 paso 1).
//     ¡Cuidado con el DEADLOCK!: usar MPI_Sendrecv, o bien ordenar por
//     paridad (pares envían primero). Documentar la elección.
//
//   static double paso_local(...)
//     El stencil serial sobre las filas propias (fila local 1 .. mis_filas),
//     leyendo las filas fantasma 0 y mis_filas+1. Devuelve suma local
//     de cuadrados (SIN sqrt: el sqrt se hace después del Allreduce).
//
// Esqueleto de run_mpi:
//
//   1. Descomposición (sección 6.1):
//        filas_int = ny - 2
//        bloque    = ceil(filas_int * 1.0 / nprocs)
//        padding   = bloque * nprocs - filas_int
//        mis_filas = (rank == nprocs-1) ? bloque - padding : bloque
//      Buffers locales de (bloque + 2) * nx, inicializados con los
//      bordes que le tocan a cada rank (rank 0: fantasma superior = 100).
//
//   2. Bucle de iteraciones (idéntico en TODOS los ranks):
//        a. MPI_Bcast de control desde rank 0: {running, pasos_a_avanzar}
//           (patrón del vector dummy de 06.fractal-mpi)
//        b. intercambiar_fantasmas(...)                  <- PUNTO A PUNTO
//        c. suma_local = paso_local(...) + swap de buffers
//        d. MPI_Allreduce(&suma_local, &suma_global, 1,
//                         MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD)  <- COLECTIVA
//           residuo = sqrt(suma_global / (nx*ny))
//           -> todos saben si parar (por eso Allreduce y no Reduce)
//        e. MPI_Gather(franja sin fantasmas, bloque*nx, MPI_DOUBLE, ...)
//           hacia rank 0 para renderizar                 <- COLECTIVA
//           (el padding hace los bloques iguales; rank 0 descarta las
//           filas de padding al pintar, como imprimir_matriz en
//           04_producto_exterior.cpp)
//
//   3. rank 0 además: ventana SFML + overlays (puede reusar las mismas
//      funciones de dibujo de main.cpp), eventos, y al cerrar difunde
//      running = 0 para apagar a los trabajadores.
//
//   4. Timing para el informe (sección 6.4): MPI_Barrier + MPI_Wtime
//      alrededor del bucle, y tres MPI_Reduce (MAX/MIN/SUM) al final —
//      patrón exacto de 05_reduce_min_max_avg.cpp.
// ============================================================

void run_mpi(const Args& args, int rank, int nprocs) {
    (void)args; // quitar al implementar
    fmt::print("run_mpi: sin implementar (rank {}/{})\n", rank, nprocs);
}
