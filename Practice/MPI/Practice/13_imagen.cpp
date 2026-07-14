#include <iostream>
#include <mpi.h>
#include <fmt/core.h>
#include <vector>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void gris(const std::vector<uint8_t>& img, std::vector<uint8_t>& res,
          int filas, int columnas) {
   for (int i = 0; i < filas; i++) {
      for (int j = 0; j < columnas; j++) {
         int index = (i * columnas + j) * 4;

         uint8_t r = img[index];
         uint8_t g = img[index + 1];
         uint8_t b = img[index + 2];
         uint8_t a = img[index + 3];

         double valor_gris = 0.21 * r + 0.72 * g + 0.07 * b;
         uint8_t gris_val = static_cast<uint8_t>(valor_gris);

         res[index]     = gris_val;
         res[index + 1] = gris_val;
         res[index + 2] = gris_val;
         res[index + 3] = a;
      }
   }
}

int main(int argc, char *argv[])
{
   MPI_Init(&argc, &argv);
   int rank, nprocs;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

   int ancho = 0;
   int alto = 0;

   unsigned char* rgba = nullptr;
   if (rank == 0) {
      int canales = 0;
      rgba = stbi_load("original.jpg", &ancho, &alto, &canales, STBI_rgb_alpha);
      if (!rgba) {
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

      int total_bytes = ancho * alto * 4;
      for (int i = 0; i < total_bytes; i++) {
         img_global[i] = rgba[i];
      }
      stbi_image_free(rgba);
   }

   if (rank == 0) {
      for (int i = 1; i < nprocs; i++) {
         MPI_Send(&img_global[i * bloque * ancho * 4], bloque * ancho * 4, MPI_UNSIGNED_CHAR, i, 2, MPI_COMM_WORLD);
      }

      for (int i = 0; i < bloque * ancho * 4; i++) {
         local_img[i] = img_global[i];
      }     
   } else {
      MPI_Recv(local_img.data(), bloque * ancho * 4, MPI_UNSIGNED_CHAR, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   }

   gris(local_img, local_res, bloque, ancho);
   
   if (rank == 0) {
      for (int i = 0; i < bloque * ancho * 4; i++) {
         res_global[i] = local_res[i];
      }
      for (int i = 1; i < nprocs; i++) {
         MPI_Recv(&res_global[i * bloque * ancho * 4], bloque * ancho * 4, MPI_UNSIGNED_CHAR, i, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }      
   } else {
      MPI_Send(local_res.data(), bloque * ancho * 4, MPI_UNSIGNED_CHAR, 0, 3, MPI_COMM_WORLD);
   }
   
   if (rank == 0) {
      std::vector<uint8_t> res_gris(ancho * alto);
      for (int i = 0; i < ancho * alto; i++) {
         res_gris[i] = res_global[i * 4];
      }
      stbi_write_png("img-gris.png", ancho, alto, STBI_grey, res_gris.data(), ancho);
      fmt::println("Imagen procesada, escala de grises generada con éxito.");
      fmt::println("Dimensiones: {}x{}", ancho, alto);
   }
   
   MPI_Finalize();
   return 0;
}