#ifndef PALETTE_H
#define PALETTE_H

#include <cstdint>
#include <vector>

// Paleta térmica frío->caliente (azul -> rojo), técnica de
// 001.fractal-Julia/palette.h pero con rampa PROPIA (generar una en
// rampgenerator.com — no reutilizar los valores del fractal).
#define PALETTE_SIZE 32

extern std::vector<uint32_t> color_ramp;

// Mapea una temperatura u en [0, u_max] al color de la rampa.
uint32_t temperatura_a_color(double u, double u_max);

#endif // PALETTE_H
