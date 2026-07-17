#include <iostream>
#include <cmath>
#include <fmt/core.h>
#include <cuda_runtime.h>

const size_t VECTOR_SIZE = 1024 * 1024;

extern void sumaVectores(float* a, float* b, float* c, int n);

int main() {
    int deviceId = 0;

    cudaSetDevice(deviceId);

    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, deviceId);

    fmt::println("Device: {}", deviceProp.name);
    fmt::println("Total memory: {:.2f} MB", deviceProp.totalGlobalMem / 1024.0 / 1024.0);
    fmt::println("Multiprocessor count: {}", deviceProp.multiProcessorCount);
    fmt::println("Max threads per multiprocessor: {}", deviceProp.maxThreadsPerMultiProcessor);
    fmt::println("Max threads per block: {}", deviceProp.maxThreadsPerBlock);
    fmt::println("Max grid size: {} x {} x {}", deviceProp.maxGridSize[0], deviceProp.maxGridSize[1], deviceProp.maxGridSize[2]);

    // -- Inicializar host
    float* h_A = new float[VECTOR_SIZE];
    float* h_B = new float[VECTOR_SIZE];
    float* h_C = new float[VECTOR_SIZE];

    for (size_t i = 0; i < VECTOR_SIZE; ++i) {
        h_A[i] = 1.0f;
        h_B[i] = 2.0f;
        h_C[i] = 0.0f;
    }

    // -- Inicializar device
    float* d_A = nullptr;
    float* d_B = nullptr;
    float* d_C = nullptr;

    size_t size_in_bytes = VECTOR_SIZE * sizeof(float);

    cudaMalloc((void**)&d_A, size_in_bytes);
    cudaMalloc((void**)&d_B, size_in_bytes);
    cudaMalloc((void**)&d_C, size_in_bytes);

    // -- Copiar del host al device
    cudaMemcpy(d_A, h_A, size_in_bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size_in_bytes, cudaMemcpyHostToDevice);

    // -- Invocar el wrapper del kernel
    sumaVectores(d_A, d_B, d_C, VECTOR_SIZE);

    // Alinear la GPU con la CPU antes de leer resultados
    cudaDeviceSynchronize();

    // --- Copiar del device al host
    cudaMemcpy(h_C, d_C, size_in_bytes, cudaMemcpyDeviceToHost);

    // -- Imprimir resultado
    for (size_t i = 0; i < 10; ++i) {
        fmt::println("C[{}] = {}", i, h_C[i]);
    }

    // -- Liberar memoria del device
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    // -- Liberar memoria del host
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;

    return 0;
}