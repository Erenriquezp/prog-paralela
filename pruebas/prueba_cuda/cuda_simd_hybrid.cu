#include <iostream>
#include <vector>
#include <cmath>
#include <cuda_runtime.h>
#include <immintrin.h> // Cabecera estándar de Intel/AMD para intrínsecos SIMD (AVX2)

// 1. KERNEL DE CUDA (Ejecución en GPU)
__global__ void multiplicar_kernel(float* d_A, float* d_B, float* d_C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        d_C[idx] = d_A[idx] * d_B[idx];
    }
}

int main() {
    // 8 millones de elementos (debe ser divisible por 8 para que SIMD calce perfectamente)
    const int N = 8000000; 
    size_t size_in_bytes = N * sizeof(float);

    std::cout << "=== INICIANDO PIPELINE HÍBRIDO (CUDA + SIMD AVX2) ===" << std::endl;

    // --- FASE 1: RESERVA EN HOST (CPU) ---
    float* h_A = new float[N];
    float* h_B = new float[N];
    float* h_C = new float[N];

    // --- FASE 2: INICIALIZACIÓN VECTORIAL EN CPU (SIMD AVX2) ---
    // En lugar de un bucle clásico de uno en uno, saltamos de 8 en 8 elementos.
    // Procesamos un lote de 256 bits completo por iteración.
    for (int i = 0; i < N; i += 8) {
        // Creamos un vector AVX2 con los índices secuenciales {i, i+1, ..., i+7}
        __m256 idx_vec = _mm256_setr_ps(i, i+1, i+2, i+3, i+4, i+5, i+6, i+7);
        
        // Multiplicamos secuencialmente los índices por 0.0001f (usando un registro con valor constante)
        __m256 val_A = _mm256_mul_ps(idx_vec, _mm256_set1_ps(0.0001f));
        
        // Creamos un registro donde los 8 carriles contienen el valor constante 2.0f
        __m256 val_B = _mm256_set1_ps(2.0f);
        
        // Almacenamos el resultado directamente de los registros SIMD a la memoria RAM normal.
        // Se usa 'storeu' (unaligned store) para evitar restricciones estrictas de alineación de memoria.
        _mm256_storeu_ps(&h_A[i], val_A);
        _mm256_storeu_ps(&h_B[i], val_B);
    }

    std::cout << "Datos preparados en CPU usando registros vectoriales AVX2 (8 floats por ciclo)..." << std::endl;

    // --- FASE 3: RESERVA Y COPIA A DEVICE (GPU) ---
    float *d_A = nullptr, *d_B = nullptr, *d_C = nullptr;
    cudaMalloc((void**)&d_A, size_in_bytes);
    cudaMalloc((void**)&d_B, size_in_bytes);
    cudaMalloc((void**)&d_C, size_in_bytes);

    cudaMemcpy(d_A, h_A, size_in_bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size_in_bytes, cudaMemcpyHostToDevice);

    // --- FASE 4: EJECUCIÓN DEL KERNEL (CUDA) ---
    int threads_per_block = 256;
    int blocks_per_grid = (N + threads_per_block - 1) / threads_per_block;

    std::cout << "Lanzando kernel en GPU con " << blocks_per_grid << " bloques de hilos..." << std::endl;
    multiplicar_kernel<<<blocks_per_grid, threads_per_block>>>(d_A, d_B, d_C, N);
    
    cudaDeviceSynchronize();

    // --- FASE 5: COPIA DEVICE A HOST (GPU ➜ CPU) ---
    cudaMemcpy(h_C, d_C, size_in_bytes, cudaMemcpyDeviceToHost);

    // --- FASE 6: VERIFICACIÓN CON REDUCCIÓN VECTORIAL (SIMD AVX2) ---
    // Usamos instrucciones intrínsecas para sumar de 8 en 8 los floats del vector resultante.
    __m256 sum_vec = _mm256_setzero_ps(); // Inicializamos el registro acumulador en 0.0f
    
    for (int i = 0; i < N; i += 8) {
        __m256 val_C = _mm256_loadu_ps(&h_C[i]); // Cargamos 8 floats a la vez
        sum_vec = _mm256_add_ps(sum_vec, val_C);  // Suma vectorial paralela (8 sumas en un ciclo)
    }

    // Para obtener la suma final, hacemos una reducción horizontal del registro acumulador
    // Extrayendo los 8 floats intermedios del registro a un arreglo local alineado
    alignas(32) float temp[8];
    _mm256_store_ps(temp, sum_vec);
    
    double suma_verificacion = 0.0;
    for (int i = 0; i < 8; i++) {
        suma_verificacion += temp[i];
    }

    // --- FASE 7: VERIFICACIÓN FINAL ---
    std::cout << "=== RESULTADOS ===" << std::endl;
    std::cout << "Muestra C[100]: " << h_C[100] << " (Esperado: " << (100 * 0.0001f * 2.0f) << ")" << std::endl;
    std::cout << "Suma acumulada calculada en CPU (SIMD AVX2): " << suma_verificacion << std::endl;

    // --- FASE 8: LIBERACIÓN DE RECURSOS ---
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;

    return 0;
}