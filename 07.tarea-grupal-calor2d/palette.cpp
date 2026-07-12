#include "palette.h"

#include <algorithm>

// ============================================================
// TODO (Integrante A) — sección 4.1 del PLAN.md:
//
// 1. Generar una rampa PROPIA de 32 colores azul->rojo en
//    https://www.rampgenerator.com/ (misma herramienta que usó el
//    profe para el fractal, pero con SUS colores: p.ej. de #0000FF
//    a #FF0000 pasando por cian/amarillo para que el mapa de calor
//    se lea bien).
//
// 2. Llenar color_ramp con los 32 valores usando el truco bswap32
//    de 001.fractal-Julia/palette.cpp (SFML espera los canales RGBA
//    en orden inverso al literal 0xRRGGBBAA).
//
// 3. Implementar temperatura_a_color:
//       int idx = (int)(u / u_max * (PALETTE_SIZE - 1));
//       idx = std::clamp(idx, 0, PALETTE_SIZE - 1);
//       return color_ramp[idx];
// ============================================================

std::vector<uint32_t> color_ramp = {
    // TODO: 32 colores propios (ver arriba)
    0xFF000000,
};

uint32_t temperatura_a_color(double u, double u_max) {
    (void)u; (void)u_max; // quitar al implementar
    return color_ramp[0];
}
