// ============================================================
// LISTADOS 8.5, 8.6 y 8.7: SEND/RECEIVE ENTRE PAREJAS SIN DEADLOCK
// Cada rank intercambia un arreglo con su pareja (0<->1, 2<->3...).
// PELIGRO: si TODOS hacen Recv primero (o Send primero con mensaje
// grande) el programa se CUELGA (deadlock). Hay 3 formas seguras:
//   A) Alternar el orden por rank par/impar        (listado 8.5)
//   B) MPI_Sendrecv: MPI se encarga del orden      (listado 8.6)
//   C) Isend/Irecv + Waitall: no bloquean          (listado 8.7)
// Ejecutar: mpiexec -n 4 02_sendrecv.exe   (nprocs debe ser PAR)
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <vector>

#define COUNT 10

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs % 2 == 1) {
        if (rank == 0) fmt::println("Debe ejecutarse con un numero PAR de procesos");
        MPI_Finalize();
        return 1;
    }

    // Mi pareja: 0<->1, 2<->3, 4<->5 ...
    int pareja = (rank / 2) * 2 + (rank + 1) % 2;
    int tag = rank / 2;                     // mismo tag para cada pareja

    std::vector<double> xsend(COUNT), xrecv(COUNT);
    for (int i = 0; i < COUNT; i++) {
        xsend[i] = rank;                    // envío mi propio rank
    }

    // --- FORMA A: alternar Send/Recv por rank par/impar (listado 8.5) ---
    if (rank % 2 == 0) {
        MPI_Send(xsend.data(), COUNT, MPI_DOUBLE, pareja, tag, MPI_COMM_WORLD);
        MPI_Recv(xrecv.data(), COUNT, MPI_DOUBLE, pareja, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(xrecv.data(), COUNT, MPI_DOUBLE, pareja, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(xsend.data(), COUNT, MPI_DOUBLE, pareja, tag, MPI_COMM_WORLD);
    }
    fmt::println("A) RANK_{} recibio {} de RANK_{}", rank, xrecv[0], pareja);

    // --- FORMA B: MPI_Sendrecv, una sola llamada combinada (listado 8.6) ---
    MPI_Sendrecv(xsend.data(), COUNT, MPI_DOUBLE, pareja, tag,
                 xrecv.data(), COUNT, MPI_DOUBLE, pareja, tag,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fmt::println("B) RANK_{} recibio {} de RANK_{}", rank, xrecv[0], pareja);

    // --- FORMA C: llamadas inmediatas (no bloquean) + Waitall (listado 8.7) ---
    MPI_Request requests[2];
    MPI_Irecv(xrecv.data(), COUNT, MPI_DOUBLE, pareja, tag, MPI_COMM_WORLD, &requests[0]);
    MPI_Isend(xsend.data(), COUNT, MPI_DOUBLE, pareja, tag, MPI_COMM_WORLD, &requests[1]);
    MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);  // esperar a que ambas terminen
    fmt::println("C) RANK_{} recibio {} de RANK_{}", rank, xrecv[0], pareja);

    MPI_Finalize();
    return 0;
}
