#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <fmt/core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void aplicar_escala_grises(const std::vector<uint8_t>& img_local,
                           std::vector<uint8_t>& res_local,
                           int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            int index = (i * columnas + j) * 4;

            uint8_t r = img_local[index];
            uint8_t g = img_local[index + 1];
            uint8_t b = img_local[index + 2];
            uint8_t a = img_local[index + 3];

            double valor_gris = 0.21 * r + 0.72 * g + 0.07 * b;
            uint8_t gris = (uint8_t)valor_gris;

            res_local[index]     = gris;
            res_local[index + 1] = gris;
            res_local[index + 2] = gris;
            res_local[index + 3] = a; 
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int ancho = 0;
    int alto = 0;

    unsigned char* rgba_raw = nullptr;
    if (rank == 0) {
        int canales = 0;
        rgba_raw = stbi_load("original.jpg", &ancho, &alto, &canales, STBI_rgb_alpha);
        if (!rgba_raw) {
            fmt::print("Error: No se pudo cargar la imagen 'original.jpg'\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }

    MPI_Bcast(&ancho, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&alto, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int bloque = std::ceil(alto * 1.0 / nprocs);
    int total_pad = bloque * nprocs;

    std::vector<uint8_t> local_img(bloque * ancho * 4, 0);
    std::vector<uint8_t> local_res(bloque * ancho * 4, 0);

    std::vector<uint8_t> img_global;
    std::vector<uint8_t> res_global;

    if (rank == 0) {
        img_global.resize(total_pad * ancho * 4, 0);
        res_global.resize(total_pad * ancho * 4, 0);

        int total_bytes_reales = ancho * alto * 4;
        for (int i = 0; i < total_bytes_reales; i++) {
            img_global[i] = rgba_raw[i];
        }
        stbi_image_free(rgba_raw);
    }

    MPI_Scatter(img_global.data(), bloque * ancho * 4, MPI_UINT8_T,
                local_img.data(), bloque * ancho * 4, MPI_UINT8_T,
                0, MPI_COMM_WORLD);

    aplicar_escala_grises(local_img, local_res, bloque, ancho);

    MPI_Gather(local_res.data(), bloque * ancho * 4, MPI_UINT8_T,
               res_global.data(), bloque * ancho * 4, MPI_UINT8_T,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<uint8_t> pixeles_gris(ancho * alto);
        for (int i = 0; i < ancho * alto; i++) {
            pixeles_gris[i] = res_global[i * 4]; 
        }

        stbi_write_png("img-gris2.png", ancho, alto, STBI_grey, pixeles_gris.data(), ancho);
        fmt::print("Imagen procesada y guardada como 'img-gris2.png' (Colectivo)\n");
        fmt::print("Dimensiones reales: {}x{}\n", ancho, alto);
    }

    MPI_Finalize();
    return 0;
}