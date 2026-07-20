#include <cuda_runtime.h>
#include <cmath>
#include <cstdint>

__global__ void mi_kernel(float *d_A, float *d_B, float *d_C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        d_A[idx] = idx;
        d_B[idx] = idx * 2.0;
        d_C[idx] = d_A[idx] + d_B[idx];
    }
}

void lanzar_kernel(float *d_A, float *d_B, float *d_C, int n) {
    int threads = 1024;
    int blocks = (n + threads - 1) / threads;

    mi_kernel<<<blocks, threads>>>(d_A, d_B, d_C, n);
}

int main() {
    int n = 10000000;
    size_t bytes = n * sizeof(float);

    float *h_A = new float[n];
    float *h_B = new float[n];
    float *h_C = new float[n];

    float *d_A, *d_B, *d_C;

    cudaMalloc(&d_A, bytes);
    cudaMalloc(&d_B, bytes);
    cudaMalloc(&d_C, bytes);

    cudaMemcpy(d_A, h_A, bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, bytes, cudaMemcpyHostToDevice);

    lanzar_kernel(d_A, d_B, d_C, n);

    cudaDeviceSynchronize();

    cudaMemcpy(h_C, d_C, bytes, cudaMemcpyDeviceToHost);

    for (int i = 0; i < 100; i++) {
        fmt::print("{}", h_C[i]);
    }

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
}