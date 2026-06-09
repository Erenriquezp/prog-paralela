#include <iostream>
#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath> // Necesario para std::ceil

#define MATRIX_DIM 25

// Función para imprimir la matriz aplanada (formato filas x columnas)
void imprimir_matriz(const std::vector<double>& matriz, int filas, int columnas) {
    fmt::print("\n--- Matriz local en RANK (imprimiendo {}x{}) ---\n", filas, columnas);
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
void imprimir_vector(const std::vector<double>& vector, const std::string& nombre = "Vector") {
    fmt::print("\n--- {} (dimension {}) ---\n", nombre, vector.size());
    for (double val : vector) {
        fmt::print("{:>5.1f} ", val);
    }
    fmt::print("\n-----------------------------------\n");
}

// Multiplica una submatriz de [rows x dim] por un vector 'x' de tamaño [dim]
void multiplicar_matriz_vector(const std::vector<double>& A_local, const std::vector<double>& x, std::vector<double>& resultado, int rows, int dim) {
    for (int i = 0; i < rows; i++) {
        resultado[i] = 0.0;
        for (int j = 0; j < dim; j++) {
            resultado[i] += A_local[i * dim + j] * x[j];
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Vector local para el vector 'b', todos los nodos lo necesitan
    std::vector<double> b_local(MATRIX_DIM);
    
    // Variables compartidas para el cálculo de la distribución
    int rows_per_rank = std::ceil(MATRIX_DIM * 1.0 / nprocs); 
    // si nprocs = 8 (o a uno que de problemas), rellenar la matriz A 32x25, b 25*1, x 32*1, no calcular las filas de relleno
    // tareal realizar la implementacion: para enviar bloques iguales, para culquier n

    int padding = rows_per_rank * nprocs - MATRIX_DIM;

    std::vector<double> A_local;
    int filas_locales = 0;
    
    // Vector global X (solo el Rank 0 acumulará aquí el resultado final)
    std::vector<double> x_global;

    if (rank == 0) {
        std::vector<double> A(MATRIX_DIM * MATRIX_DIM);
        std::vector<double> b(MATRIX_DIM);
        x_global.resize(MATRIX_DIM); // Inicializar tamaño para el resultado final

        // Inicializar Matriz A
        for (int i = 0; i < MATRIX_DIM; i++) {
            for (int j = 0; j < MATRIX_DIM; j++) {
                int index = i * MATRIX_DIM + j;
                A[index] = i; // Rellena con el índice de la fila
            }            
        }

        // Inicializar Vector b
        for (int i = 0; i < MATRIX_DIM; i++) {
            b[i] = 1;           
        }
        
        // Copiar el b local del rank 0
        b_local = b;
        
        fmt::print("MATRIX_DIM: {}, nprocs: {}, rows_per_rank: {}, padding: {}\n",
            MATRIX_DIM, nprocs, rows_per_rank, padding
        );

        // Enviar datos a los esclavos (ranks 1 hasta nprocs-1)
        for (int i = 1; i < nprocs; i++) {
            int filas = rows_per_rank;
            if (i == nprocs - 1) {
                filas = rows_per_rank - padding;
            }

            // Enviar las dimensiones
            std::vector<int> data = {MATRIX_DIM, filas};
            MPI_Send(data.data(), data.size(), MPI_INT, i, 0, MPI_COMM_WORLD);

            // Enviar los datos de la matriz A
            const double* buffer = A.data();
            MPI_Send(&buffer[i * rows_per_rank * MATRIX_DIM], filas * MATRIX_DIM, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }

        // El Rank 0 procesa sus propias filas locales de A
        filas_locales = rows_per_rank;
        A_local.resize(filas_locales * MATRIX_DIM);
        std::copy(A.begin(), A.begin() + (filas_locales * MATRIX_DIM), A_local.begin());

        fmt::println("RANK_{}, {} x {}", rank, filas_locales, MATRIX_DIM);

    } else {
        // Bloque para los Ranks esclavos (rank != 0)
        std::vector<int> data_rec(2);
        MPI_Recv(
            data_rec.data(), 
            2, 
            MPI_INT, 
            0, 
            0, 
            MPI_COMM_WORLD, 
            MPI_STATUS_IGNORE
        );
        int matrix_dim = data_rec[0];
        filas_locales = data_rec[1];

        fmt::print("RANK_{}, {} x {}\n", rank, filas_locales, matrix_dim);
        A_local.resize(filas_locales * matrix_dim);

        // Recibir sub-matriz A
        MPI_Recv(
            A_local.data(), 
            filas_locales * matrix_dim, 
            MPI_DOUBLE, 
            0, 
            0, 
            MPI_COMM_WORLD, 
            MPI_STATUS_IGNORE
        );
    }

    // Difusión colectiva del vector 'b'
    MPI_Bcast(b_local.data(), MATRIX_DIM, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Imprimir la matriz si cumple la condición del Rank 2
    //if (rank == 2) {
       // imprimir_matriz(A_local, filas_locales, MATRIX_DIM);
        //imprimir_vector(b_local, "b_local en Rank 2");
    //}

    // Cada rank calcula su porción del resultado final
    std::vector<double> x_local(filas_locales);
    multiplicar_matriz_vector(A_local, b_local, x_local, filas_locales, MATRIX_DIM);
    
    // Imprimir lo que calculó cada rank de manera individual
    //imprimir_vector(x_local, fmt::format("x_local en RANK_{}", rank));

    // --- RECOLECCIÓN DE DATOS (Gather Manual) ---
    if (rank == 0) {
        // 1. El Rank 0 copia directamente su propio resultado local
        std::copy(x_local.begin(), x_local.end(), x_global.begin());

        // 2. El Rank 0 recibe las partes de los demás Ranks
        for (int i = 1; i < nprocs; i++) {
            int filas = rows_per_rank;
            if (i == nprocs - 1) {
                filas = rows_per_rank - padding;
            }   
            MPI_Recv(
                x_global.data() + (i * rows_per_rank),
                filas,
                MPI_DOUBLE,
                i,
                1, // Usamos un tag diferente (1) para la recolección
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE
            );     
        }

        // Imprimir el resultado definitivo consolidado
        fmt::println("\n=================================");
        imprimir_vector(x_global, "RESULTADO GLOBAL COMPLETO (X)");
        fmt::println("=================================");

    } else {
        // Los esclavos envían su trozo calculado al Rank 0
        MPI_Send(
            x_local.data(),
            filas_locales,
            MPI_DOUBLE,
            0,
            1, // Tag de recolección coincidente
            MPI_COMM_WORLD
        );
    }

    MPI_Finalize();
    return 0;
}