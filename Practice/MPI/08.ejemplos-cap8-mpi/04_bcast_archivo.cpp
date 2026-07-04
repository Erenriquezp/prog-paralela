// ============================================================
// LISTADO 8.10: BROADCAST PARA LEER UN ARCHIVO PEQUEÑO
// Solo el rank 0 lee el archivo (los discos son seriales y lentos)
// y luego lo difunde a todos en DOS pasos:
//   1. Bcast del TAMAÑO (para que cada uno reserve memoria)
//   2. Bcast del CONTENIDO
// Ejecutar desde esta carpeta (aquí está entrada.txt):
//   mpiexec -n 4 build\Debug\04_bcast_archivo.exe
// ============================================================
#include <fmt/core.h>
#include <mpi.h>
#include <fstream>
#include <string>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string contenido;
    int tam = 0;

    // Solo el rank 0 lee el archivo completo
    if (rank == 0) {
        std::ifstream archivo("entrada.txt");
        std::string linea;
        while (std::getline(archivo, linea)) {
            contenido = contenido + linea + "\n";
        }
        tam = contenido.size();
    }

    // 1. Difundir el tamaño para que todos reserven memoria
    MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);
    contenido.resize(tam);

    // 2. Difundir el contenido completo
    MPI_Bcast(contenido.data(), tam, MPI_CHAR, 0, MPI_COMM_WORLD);

    fmt::print("RANK_{} leyo {} caracteres:\n{}", rank, tam, contenido);

    MPI_Finalize();
    return 0;
}
