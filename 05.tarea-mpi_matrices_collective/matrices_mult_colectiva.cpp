#include <iostream>
#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath> 

#define MATRIX_DIM 25

// Función auxiliar para imprimir un vector (solo hasta la dimensión real)
void imprimir_vector_real(const std::vector<double>& vector, int elementos_reales, int rank) {
    fmt::print("\n--- Vector en RANK_{} ({} elementos reales) ---\n", rank, elementos_reales);
    for (int i = 0; i < elementos_reales; i++) {
        fmt::print("{:>5.1f} ", vector[i]);
    }
    fmt::print("\n-------------------------------------------------\n");
}

// Función matemática protegida contra el padding
void multiplicar_matriz_vector_padding(const std::vector<double>& matriz,
                                       const std::vector<double>& vector,
                                       std::vector<double>& resultado,
                                       int filas_locales,
                                       int columnas,
                                       int rank,
                                       int dim_real) {
    for (int i = 0; i < filas_locales; i++) {
        int global_row_index = (rank * filas_locales) + i;

        if (global_row_index < dim_real) {
            double sum = 0.0;
            for (int j = 0; j < columnas; j++) {
                int index = i * columnas + j;
                sum += matriz[index] * vector[j];
            }
            resultado[i] = sum;
        } else {
            resultado[i] = 0.0; // Fila de relleno (Padding)
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // 1. CÁLCULO UNIFORME PARA TODOS LOS NODOS
    int rows_per_rank = std::ceil(MATRIX_DIM * 1.0 / nprocs);
    int padded_rows = rows_per_rank * nprocs;

    // Reservas de memoria que TODOS los procesos necesitan
    std::vector<double> b_local(MATRIX_DIM);
    std::vector<double> A_local(rows_per_rank * MATRIX_DIM);
    std::vector<double> x_local(rows_per_rank);

    // Variables exclusivas del Maestro
    std::vector<double> A_global;
    std::vector<double> x_global; 

    // --- FASE 1: INICIALIZACIÓN (Solo el Maestro) ---
    if (rank == 0) {
        A_global.resize(padded_rows * MATRIX_DIM, 0.0);
        x_global.resize(padded_rows, 0.0);

        for (int i = 0; i < MATRIX_DIM; i++) {
            b_local[i] = 1.0; 
            for (int j = 0; j < MATRIX_DIM; j++) {
                A_global[i * MATRIX_DIM + j] = i; 
            }            
        }

        fmt::print("Dimension Real: {}, Nodos: {}, Filas/Nodo: {}, Matriz Aplanada con Padding: {}x{}\n",
            MATRIX_DIM, nprocs, rows_per_rank, padded_rows, MATRIX_DIM);
    }

    // --- FASE 2: COMUNICACIÓN COLECTIVA (Distribución) ---
    // Todos los procesos deben ejecutar estas dos líneas sin 'if'

    // 1. Difusión: El Rank 0 envía el vector b a TODOS
    MPI_Bcast(b_local.data(), MATRIX_DIM, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 2. Dispersión: El Rank 0 divide la matriz A_global y reparte a los A_local
    MPI_Scatter(
        rank == 0 ? A_global.data() : nullptr, // Buffer origen (solo válido en Maestro)
        rows_per_rank * MATRIX_DIM,            // Cantidad a ENVIAR a cada proceso
        MPI_DOUBLE,                            // Tipo de dato a enviar
        A_local.data(),                        // Buffer destino (válido en todos)
        rows_per_rank * MATRIX_DIM,            // Cantidad a RECIBIR por cada proceso
        MPI_DOUBLE,                            // Tipo de dato a recibir
        0,                                     // ¿Quién es el origen? (Rank 0)
        MPI_COMM_WORLD                         // Comunicador
    );

    // --- FASE 3: CÁLCULO PARALELO ---
    multiplicar_matriz_vector_padding(A_local, b_local, x_local, rows_per_rank, MATRIX_DIM, rank, MATRIX_DIM);

    // --- FASE 4: COMUNICACIÓN COLECTIVA (Recolección) ---
    // 3. Agrupación: Todos envían su x_local y el Rank 0 los ensambla en x_global
    MPI_Gather(
        x_local.data(),                        // Buffer origen (lo que calculó cada uno)
        rows_per_rank,                         // Cantidad enviada por cada uno
        MPI_DOUBLE,                            // Tipo de dato enviado
        rank == 0 ? x_global.data() : nullptr, // Buffer destino (solo válido en Maestro)
        rows_per_rank,                         // Cantidad que recibirá de cada uno
        MPI_DOUBLE,                            // Tipo de dato recibido
        0,                                     // ¿Quién es el destino? (Rank 0)
        MPI_COMM_WORLD                         // Comunicador
    );

    // --- FASE 5: IMPRESIÓN ---
    if (rank == 0) {
        fmt::print("\n========== RESULTADO GLOBAL (CON COLECTIVAS) ==========\n");
        imprimir_vector_real(x_global, MATRIX_DIM, rank);
    }
    
    MPI_Finalize();
    return 0;
}