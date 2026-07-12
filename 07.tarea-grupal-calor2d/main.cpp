#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <mpi.h>
#include <omp.h>
#include <vector>

#include "args.h"
#include "palette.h"
#include "heat_serial.h"
#include "heat_simd.h"
#include "heat_openmp.h"
#include "heat_mpi.h"

#ifdef _WIN32
    #include <windows.h>
#endif

// Ventana fija del enunciado (4.1); la grilla nx x ny se escala a ella.
#define WIN_W 1600
#define WIN_H 900

// ============================================================
// TODO (Integrante A) — secciones 3 y 4 del PLAN.md.
// Referencia de patrón: 001.fractal-Julia/main.cpp (REESCRIBIR, no copiar:
// paleta térmica propia, controles propios, textos propios).
//
// Funciones sugeridas:
//
//   void init_campo(std::vector<double>& u, int nx, int ny)
//     Todo a 0.0 y la fila superior (j = 0) a 100.0. Se aplica a
//     AMBOS buffers (u y u_new) — ver sección 2.2 del PLAN.md.
//
//   void campo_a_pixeles(const std::vector<double>& u, uint32_t* buf, int nx, int ny)
//     buf[j*nx+i] = temperatura_a_color(u[j*nx+i], 100.0);
//
// Flujo de main:
//
//   1. parse_args; si falla, imprimir uso y salir.
//
//   2. Si backend == "mpi": MPI_Init -> rank/nprocs -> run_mpi(...)
//      -> MPI_Finalize -> return. Todo lo demás es el camino no-MPI.
//
//   3. Buffers: std::vector<double> u, u_new (nx*ny) + init_campo en ambos;
//      uint32_t* pixel_buffer de nx*ny.
//
//   4. SFML (API 3, como el fractal):
//      - RenderWindow con VideoMode({WIN_W, WIN_H})
//      - sf::Texture de {nx, ny} + sf::Sprite escalado:
//          sprite.setScale({(float)WIN_W / nx, (float)WIN_H / ny});
//      - sf::Font("arial.ttf") + dos sf::Text:
//          overlay superior (10,10): backend, iter, residuo, MFLOPS, FPS
//          overlay inferior (10, WIN_H-40): menú de teclas
//
//   5. Estado de la simulación:
//        int iter_actual = 0; double residuo = 1e9;
//        bool en_pausa = true; bool un_paso = false;
//        enum backend_type { SERIAL, SIMD, OPENMP } activo;
//
//   6. Bucle while (window.isOpen()):
//      a. Eventos (patrón SFML 3 del fractal):
//           Closed -> close
//           KeyReleased: Num1/2/3 -> cambiar backend
//                        Space -> en_pausa = !en_pausa
//                        S -> un_paso = true
//                        R -> init_campo en ambos buffers, iter_actual = 0
//      b. Si (!en_pausa || un_paso) y no se cumplió la parada
//         (iter_actual < args.iter && residuo > args.tol):
//           - medir t0 (std::chrono::steady_clock)
//           - avanzar K iteraciones (K=10 en continuo, K=1 si un_paso):
//               residuo = heat_step_XXX(u.data(), u_new.data(), nx, ny, r);
//               std::swap(u, u_new);
//               iter_actual++;
//           - medir t1 y actualizar MFLOPS (sección 2.5):
//               10.0 * (nx-2) * (ny-2) * K / segundos / 1e6
//           - un_paso = false;
//      c. campo_a_pixeles + texture.update((const uint8_t*)pixel_buffer)
//      d. FPS con sf::Clock (mismo contador del fractal), actualizar
//         los dos textos con fmt::format, clear/draw/display.
//
//   7. Para OpenMP: obtener thread_count al inicio con
//      #pragma omp parallel + #pragma omp master (truco del fractal)
//      y mostrarlo en el overlay cuando el backend activo sea OPENMP.
// ============================================================

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) {
        return 1;
    }

    // TODO: implementar el flujo descrito arriba
    fmt::print("main: sin implementar (backend={})\n", args.backend);

    return 0;
}
