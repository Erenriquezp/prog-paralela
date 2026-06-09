#include <iostream>
#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath> // Necesario para std::ceil

#define MATRIX_DIM 25

// Función para imprimir la matriz aplanada (formato filas x columnas)
void imprimir_matriz(const std::vector<double>& matriz, int filas, int columnas, int rank) {
    fmt::print("\n--- Matriz local en RANK_{} (imprimiendo {}x{}) ---\n", rank, filas, columnas);
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            int index = i * columnas + j;
            fmt::print("{:>5.1f} ", matriz[index]);
        }
        fmt::print("\n");
    }
    fmt::print("-------------------------------------------------\n");
}

// Función auxiliar para imprimir un vector
void imprimir_vector(const std::vector<double>& vector, int elementos, int rank) {
    fmt::print("\n--- Vector en RANK_{} ({} elementos) ---\n", rank, elementos);
    for (int i = 0; i < elementos; i++) {
        fmt::print("{:>5.1f} ", vector[i]);
    }
    fmt::print("\n-------------------------------------------------\n");
}

// Función secuencial para multiplicar matriz por vector
void multiplicar_matriz_vector(const std::vector<double>& matriz,
                               const std::vector<double>& vector,
                               std::vector<double>& resultado,
                               int filas,
                               int columnas) {
    for (int i = 0; i < filas; i++) {
        double sum = 0.0;
        for (int j = 0; j < columnas; j++) {
            int index = i * columnas + j;
            sum += matriz[index] * vector[j];
        }
        resultado[i] = sum;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Variables globales que todo nodo necesita para trabajar
    std::vector<double> b_local(MATRIX_DIM);
    std::vector<double> A_local;
    std::vector<double> x_local;
    int filas_locales = 0;

    // Variables exclusivas del Maestro (Rank 0)
    std::vector<double> x_global; 

    // --- FASE 1: INICIALIZACIÓN Y DISTRIBUCIÓN ---
    if (rank == 0) {
        std::vector<double> A(MATRIX_DIM * MATRIX_DIM);
        std::vector<double> b(MATRIX_DIM);
        x_global.resize(MATRIX_DIM);

        // Inicializar Matriz A y Vector b
        for (int i = 0; i < MATRIX_DIM; i++) {
            b[i] = 1.0; 
            for (int j = 0; j < MATRIX_DIM; j++) {
                int index = i * MATRIX_DIM + j;
                A[index] = i; // Rellena con el índice de la fila
            }            
        }
        
        b_local = b; // El Rank 0 guarda su propia copia
        
        int rows_per_rank = std::ceil(MATRIX_DIM * 1.0 / nprocs);
        int padding = rows_per_rank * nprocs - MATRIX_DIM;

        fmt::print("MATRIX_DIM: {}, nprocs: {}, rows_per_rank: {}, padding: {}\n",
            MATRIX_DIM, nprocs, rows_per_rank, padding);

        // Distribución MANUAL (Punto a Punto) a los esclavos
        for (int i = 1; i < nprocs; i++) {
            int filas = rows_per_rank;
            if (i == nprocs - 1) {
                filas = rows_per_rank - padding;
            }

            // 1. Enviar metadata (dimensiones)
            std::vector<int> data = {MATRIX_DIM, filas};
            MPI_Send(data.data(), data.size(), MPI_INT, i, 0, MPI_COMM_WORLD);

            // 2. Enviar bloque de la Matriz A
            const double* buffer = A.data();
            MPI_Send(&buffer[i * rows_per_rank * MATRIX_DIM], filas * MATRIX_DIM, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);

            // 3. Enviar el Vector b completo MANUALMENTE (Reemplazo del Bcast)
            MPI_Send(b.data(), MATRIX_DIM, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }

        // El Rank 0 prepara su propia carga de trabajo
        filas_locales = rows_per_rank;
        A_local.resize(filas_locales * MATRIX_DIM);
        std::copy(A.begin(), A.begin() + (filas_locales * MATRIX_DIM), A_local.begin());

    } else {
        // --- Recepción MANUAL de los Ranks esclavos ---
        
        // 1. Recibir metadata
        std::vector<int> data_rec(2);
        MPI_Recv(data_rec.data(), 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int matrix_dim = data_rec[0];
        filas_locales = data_rec[1];

        A_local.resize(filas_locales * matrix_dim);

        // 2. Recibir bloque de la Matriz A
        MPI_Recv(A_local.data(), filas_locales * matrix_dim, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // 3. Recibir el Vector b completo MANUALMENTE (Reemplazo del Bcast)
        MPI_Recv(b_local.data(), MATRIX_DIM, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // --- FASE 2: CÁLCULO PARALELO ---
    // En este punto, TODOS los nodos tienen su bloque de A_local y el vector b_local completo.
    
    x_local.resize(filas_locales);
    multiplicar_matriz_vector(A_local, b_local, x_local, filas_locales, MATRIX_DIM);

    // --- FASE 3: RECOLECCIÓN (GATHER MANUAL PUNTO A PUNTO) ---
    if (rank == 0) {
        int rows_per_rank = std::ceil(MATRIX_DIM * 1.0 / nprocs);
        int padding = rows_per_rank * nprocs - MATRIX_DIM;

        // El Rank 0 copia su propio resultado local al inicio del arreglo global
        std::copy(x_local.begin(), x_local.end(), x_global.begin());

        // El Rank 0 recibe los trozos de los esclavos MANUALMENTE
        for (int i = 1; i < nprocs; i++) {
            int filas = rows_per_rank;
            if (i == nprocs - 1) {
                filas = rows_per_rank - padding;
            }

            // Usamos Tag 1 para la recolección y evitar choques con la inicialización
            MPI_Recv(&x_global[i * rows_per_rank], filas, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Impresión final
        fmt::print("\n========== RESULTADO GLOBAL COMPLETO (RANK 0) ==========\n");
        imprimir_vector(x_global, MATRIX_DIM, rank);

    } else {
        // Los esclavos envían su trozo de vuelta al Rank 0
        MPI_Send(x_local.data(), filas_locales, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
    return 0;
}