#ifndef ARGS_H
#define ARGS_H

#include <string>

// Parámetros de la simulación (defaults de la tabla 2.1 del enunciado)
struct Args {
    int    nx    = 1024;      // --nx      celdas en x
    int    ny    = 1024;      // --ny      celdas en y
    double lx    = 1.0;       // --lx      longitud física en x
    double ly    = 1.0;       // --ly      longitud física en y
    double alpha = 0.25;      // --alpha   difusividad térmica
    double dt    = 5.0e-7;    // --dt      paso temporal
    int    iter  = 1000;      // --iter    iteraciones máximas
    double tol   = 1.0e-4;    // --tol     parada por residuo L2
    std::string backend = "serial";  // --backend serial|simd|openmp|mpi

    // Derivados (calcular en parse_args después de leer los flags)
    double h = 0.0;           // lx / nx
    double r = 0.0;           // alpha * dt / h^2  — debe cumplir r <= 0.25 (CFL)
};

// Parsea argv. Devuelve false (y explica por qué) si un flag es inválido
// o si no se cumple la condición CFL.
bool parse_args(int argc, char** argv, Args& args);

#endif // ARGS_H
