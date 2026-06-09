#include <iostream>
#include <fmt/core.h>
#include <mpi.h>
#include <vector>
#include <cmath> // Necesario para std::ceil

#define MATRIX_DIM 25

// Función auxiliar para imprimir un vector (solo hasta la dimensión real)
void imprimir_vector_real(const std::vector<double> &vector, int elementos_reales, int rank)
{
    fmt::print("\n--- Vector en RANK_{} ({} elementos reales) ---\n", rank, elementos_reales);
    for (int i = 0; i < elementos_reales; i++)
    {
        fmt::print("{:>5.1f} ", vector[i]);
    }
    fmt::print("\n-------------------------------------------------\n");
}

// Nueva función de multiplicación que detecta e ignora las filas de "relleno"
void multiplicar_matriz_vector_padding(const std::vector<double> &matriz,
                                       const std::vector<double> &vector,
                                       std::vector<double> &resultado,
                                       int filas_locales,
                                       int columnas,
                                       int rank,
                                       int dim_real)
{
    for (int i = 0; i < filas_locales; i++)
    {
        // Calcular a qué fila de la matriz original (global) corresponde esta fila local
        int global_row_index = (rank * filas_locales) + i;

        // Condición clave de la tarea: "No calcular las filas de relleno"
        if (global_row_index < dim_real)
        {
            double sum = 0.0;
            for (int j = 0; j < columnas; j++)
            {
                int index = i * columnas + j;
                sum += matriz[index] * vector[j];
            }
            resultado[i] = sum;
        }
        else
        {
            // Es una fila fantasma (padding), la ignoramos y asignamos 0
            resultado[i] = 0.0;
        }
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int nprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // 1. CÁLCULO UNIFORME PARA TODOS LOS NODOS
    // Cada nodo tendrá exactamente el mismo número de filas, sin excepciones.
    int rows_per_rank = std::ceil(MATRIX_DIM * 1.0 / nprocs);
    int padded_rows = rows_per_rank * nprocs; // Ej: 4 * 8 = 32 filas totales

    std::vector<double> b_local(MATRIX_DIM);
    std::vector<double> A_local(rows_per_rank * MATRIX_DIM);
    std::vector<double> x_local(rows_per_rank);

    std::vector<double> x_global;

    // --- FASE 1: INICIALIZACIÓN CON PADDING EN EL MAESTRO ---
    if (rank == 0)
    {
        // La matriz A crece a [padded_rows x MATRIX_DIM] e inicializa todo en 0.0
        std::vector<double> A(padded_rows * MATRIX_DIM, 0.0);
        std::vector<double> b(MATRIX_DIM, 1.0);

        // El vector resultado global también debe tener espacio para recibir las filas extra
        x_global.resize(padded_rows, 0.0);

        // Inicializar SOLO las filas reales (hasta MATRIX_DIM).
        // Las filas de 25 a 31 se quedan en 0.0 automáticamente.
        for (int i = 0; i < MATRIX_DIM; i++)
        {
            for (int j = 0; j < MATRIX_DIM; j++)
            {
                int index = i * MATRIX_DIM + j;
                A[index] = i;
            }
        }
        b_local = b;

        fmt::print("Dimension Real: {}, Nodos: {}, Filas/Nodo: {}, Matriz Aplanada con Padding: {}x{}\n",
                   MATRIX_DIM, nprocs, rows_per_rank, padded_rows, MATRIX_DIM);

        // Distribución MANUAL donde TODOS reciben el MISMO tamaño (rows_per_rank)
        for (int i = 1; i < nprocs; i++)
        {
            // Ya no calculamos "filas = rows_per_rank - padding", el bloque es fijo.
            const double *buffer = A.data();
            MPI_Send(&buffer[i * rows_per_rank * MATRIX_DIM], rows_per_rank * MATRIX_DIM, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
            MPI_Send(b.data(), MATRIX_DIM, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }

        // El Rank 0 toma su bloque
        std::copy(A.begin(), A.begin() + (rows_per_rank * MATRIX_DIM), A_local.begin());
    }
    else
    {
        // --- Recepción MANUAL de los Ranks esclavos ---
        // Como todos los bloques son iguales, no necesitamos recibir "metadata" primero.
        // Sabemos exactamente que recibiremos 'rows_per_rank * MATRIX_DIM' elementos.

        MPI_Recv(A_local.data(), rows_per_rank * MATRIX_DIM, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(b_local.data(), MATRIX_DIM, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // --- FASE 2: CÁLCULO PARALELO PROTEGIDO ---
    // Pasamos el rank para que la función sepa si está procesando filas fantasma.
    multiplicar_matriz_vector_padding(A_local, b_local, x_local, rows_per_rank, MATRIX_DIM, rank, MATRIX_DIM);

    // --- FASE 3: RECOLECCIÓN (GATHER MANUAL SIMÉTRICO) ---
    if (rank == 0)
    {
        std::copy(x_local.begin(), x_local.end(), x_global.begin());

        for (int i = 1; i < nprocs; i++)
        {
            // Recibe bloques del MISMO tamaño para todos
            MPI_Recv(&x_global[i * rows_per_rank], rows_per_rank, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Imprimir SOLAMENTE los primeros 25 elementos (MATRIX_DIM), ignorando la basura del padding
        fmt::print("\n========== RESULTADO GLOBAL (IGNORANDO PADDING) ==========\n");
        imprimir_vector_real(x_global, MATRIX_DIM, rank);
    }
    else
    {
        MPI_Send(x_local.data(), rows_per_rank, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

// C:\oneAPI>setvars
// C:\tools\prog-paralela\05.ejemplo-mpi\build\Debug>mpiexec -n 4 ejemplo-mpi.exe
// https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html