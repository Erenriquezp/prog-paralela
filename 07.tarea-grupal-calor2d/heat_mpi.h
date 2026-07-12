#ifndef HEAT_MPI_H
#define HEAT_MPI_H

#include "args.h"

// Punto de entrada del backend MPI (sección 6 del PLAN.md).
// Se llama desde main() cuando args.backend == "mpi", DESPUÉS de MPI_Init.
// - rank 0: ventana SFML + su franja de cómputo + recolección y render.
// - ranks > 0: bucle de trabajo (Bcast control -> fantasmas -> stencil
//   -> Allreduce residuo -> Gather) hasta recibir running = 0.
void run_mpi(const Args& args, int rank, int nprocs);

#endif // HEAT_MPI_H
