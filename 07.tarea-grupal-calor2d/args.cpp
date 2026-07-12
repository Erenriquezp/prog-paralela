#include "args.h"

#include <fmt/core.h>

bool parse_args(int argc, char** argv, Args& args) {
    // ============================================================
    // TODO (Integrante A):
    //
    // 1. Recorrer argv con un bucle: for (int i = 1; i < argc; i++).
    //    Comparar cada argumento con los flags ("--nx", "--ny", "--lx",
    //    "--ly", "--alpha", "--dt", "--iter", "--tol", "--backend") y
    //    leer el valor siguiente (argv[i+1]) con std::stoi / std::stod.
    //    Flag desconocido -> imprimir uso con fmt::print y devolver false.
    //
    // 2. Validar: nx > 2, ny > 2, iter > 0, backend es uno de los 4 válidos.
    //
    // 3. Calcular los derivados:
    //       args.h = args.lx / args.nx;
    //       args.r = args.alpha * args.dt / (args.h * args.h);
    //
    // 4. Condición CFL (sección 2.3 del PLAN.md): si args.r > 0.25,
    //    imprimir el valor de r y devolver false. Con los defaults
    //    debe dar r ≈ 0.131 (verificarlo imprimiéndolo al arrancar).
    // ============================================================

    (void)argc; (void)argv; (void)args; // quitar al implementar
    fmt::print("parse_args: sin implementar\n");
    return false;
}
