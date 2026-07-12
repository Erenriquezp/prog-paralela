#include "heat_serial.h"

#include <cmath>

double heat_step_serial(const double* u, double* u_new, int nx, int ny, double r) {
    // ============================================================
    // TODO (Integrante A) — sección 2.1 y 2.4 del PLAN.md:
    //
    // 1. Acumulador del residuo: double suma = 0.0;
    //
    // 2. Doble bucle SOLO sobre interiores:
    //       for j = 1 .. ny-2
    //         for i = 1 .. nx-2
    //    El campo está aplanado por filas: u[j*nx + i].
    //    Vecinos: izq u[j*nx+i-1], der u[j*nx+i+1],
    //             arriba u[(j-1)*nx+i], abajo u[(j+1)*nx+i].
    //
    // 3. Aplicar el esquema de 5 puntos:
    //       u_new[idx] = u[idx] + r * (vecinos - 4*u[idx])
    //
    // 4. Acumular el residuo:
    //       diff = u_new[idx] - u[idx];  suma += diff*diff;
    //
    // 5. return std::sqrt(suma / (nx*ny));
    //
    // OJO: quien llama hace std::swap(u, u_new) después; esta función
    // NO copia bordes (los bordes ya están fijados en ambos buffers
    // desde la inicialización — ver init_campo en main.cpp).
    // ============================================================

    (void)u; (void)u_new; (void)nx; (void)ny; (void)r; // quitar al implementar
    return 0.0;
}
