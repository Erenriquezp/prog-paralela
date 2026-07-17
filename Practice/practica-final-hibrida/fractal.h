#ifndef FRACTAL_H
#define FRACTAL_H

#include <cstdint>

enum Mode {
    MODE_SERIAL = 0,
    MODE_OPENMP = 1,
    MODE_CUDA = 2
};

// Funciones para CPU (Serial y OpenMP)
void newton_cpu(double x_min, double y_min, double x_max, double y_max,
                uint32_t width, uint32_t height,
                uint32_t row_start, uint32_t row_end,
                uint32_t* pixel_buffer, long long& total_iters, bool use_openmp);

// Wrappers para GPU (CUDA)
extern "C" void copiar_paleta(unsigned int* h_palette);
extern "C" void newton_gpu(int max_iter,
                           double x_min, double y_min, double x_max, double y_max,
                           uint32_t width, uint32_t height,
                           uint32_t row_start, uint32_t row_end,
                           uint32_t* device_pixels, long long* d_iters,
                           uint32_t* host_pixels, long long& total_iters);

#endif // FRACTAL_H
