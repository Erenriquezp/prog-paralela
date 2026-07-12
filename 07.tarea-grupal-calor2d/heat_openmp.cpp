#include "heat_openmp.h"

#include <cmath>
#include <omp.h>

double heat_step_openmp_regiones(const double* u, double* u_new, int nx, int ny, double r) {
    // ============================================================
    // TODO (Integrante B) — sección 5.3 del PLAN.md.
    // Referencia de patrón: julia_openmp_regiones en
    // 001.fractal-Julia/fractal_openmp.cpp (reparto manual por bloques).
    //
    // 1. double suma = 0.0;  (compartida)
    //
    // 2. #pragma omp parallel
    //    {
    //       - thread_id / thread_count con omp_get_thread_num/num_threads
    //       - repartir las filas INTERIORES (ny-2 filas, de 1 a ny-2):
    //           delta = ceil((ny-2) * 1.0 / thread_count)
    //           j_ini = 1 + thread_id * delta
    //           j_fin = j_ini + delta   (el último hilo recorta a ny-1)
    //       - double suma_local = 0.0;  (privada: se declara DENTRO)
    //       - doble bucle con el mismo stencil del serial, acumulando
    //         en suma_local
    //       - al final: #pragma omp atomic
    //                   suma += suma_local;
    //         (una sola operación atómica por hilo — barato)
    //    }
    //
    // 3. return std::sqrt(suma / (nx*ny));
    //
    // NO fijar el número de hilos en el código: OMP_NUM_THREADS debe
    // mandar (requisito explícito de la rúbrica, criterio 3).
    // ============================================================

    (void)u; (void)u_new; (void)nx; (void)ny; (void)r; // quitar al implementar
    return 0.0;
}

double heat_step_openmp_for(const double* u, double* u_new, int nx, int ny, double r) {
    // ============================================================
    // TODO (Integrante B):
    //
    // El mismo doble bucle del serial, con:
    //    #pragma omp parallel for reduction(+:suma)
    // sobre el bucle EXTERNO j (filas). La cláusula reduction hace
    // exactamente lo que la variante manual hace a mano — ese
    // contraste va al informe (patrón de reducción, criterio 5).
    // ============================================================

    (void)u; (void)u_new; (void)nx; (void)ny; (void)r; // quitar al implementar
    return 0.0;
}
