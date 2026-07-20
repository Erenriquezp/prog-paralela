#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>
#include <cuda_runtime.h>

__global__ void multiplicar_kernel(float* d_A, float* d_B, float* d_C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        d_C[idx] = d_A[idx] * d_B[idx];
    }
}

int main() {
    const int N = 10000000;
    size_t size_in_bytes = N * sizeof(float);

    std::cout << "=== INICIANDO PIPELINE HÍBRIDO (OpenMP + CUDA) ===" << std::endl;

    float* h_A = new float[N];
    float* h_B = new float[N];
    float* h_C = new float[N];

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int num_threads = omp_get_num_threads();
        
        #pragma omp single
        {
            std::cout << "Preparando datos en CPU usando " << num_threads << " hilos de OpenMP..." << std::endl;
        }

        #pragma omp for
        for (int i = 0; i < N; i++) {
            h_A[i] = i * 0.0001f;
            h_B[i] = 2.0f;
        }
    }

    float *d_A = nullptr, *d_B = nullptr, *d_C = nullptr;
    cudaMalloc((void**)&d_A, size_in_bytes);
    cudaMalloc((void**)&d_B, size_in_bytes);
    cudaMalloc((void**)&d_C, size_in_bytes);
    cudaMemcpy(d_A, h_A, size_in_bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size_in_bytes, cudaMemcpyHostToDevice);

    int threads_per_block = 256;
    int blocks_per_grid = (N + threads_per_block - 1) / threads_per_block;

    std::cout << "Lanzando kernel en GPU con " << blocks_per_grid << " bloques de " << threads_per_block << " hilos..." << std::endl;
    
    multiplicar_kernel<<<blocks_per_grid, threads_per_block>>>(d_A, d_B, d_C, N);
    
    cudaDeviceSynchronize();

    cudaMemcpy(h_C, d_C, size_in_bytes, cudaMemcpyDeviceToHost);

    double suma_verificacion = 0.0;
    
    #pragma omp parallel for reduction(+:suma_verificacion)
    for (int i = 0; i < N; i++) {
        suma_verificacion += h_C[i];
    }

    std::cout << "=== RESULTADOS ===" << std::endl;
    std::cout << "Muestra de salida: C[100] = " << h_C[100] << " (Esperado: " << (100 * 0.0001f * 2.0f) << ")" << std::endl;
    std::cout << "Suma total de verificacion calculada en CPU (OpenMP): " << suma_verificacion << std::endl;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;

    return 0;
}