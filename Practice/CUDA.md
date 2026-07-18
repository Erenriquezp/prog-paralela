# Guía de Estudio CUDA

---

## 1. ¿Qué es CUDA?

CUDA permite ejecutar código en la **GPU** (tarjeta gráfica) en lugar de la **CPU**. La GPU tiene miles de núcleos pequeños que ejecutan la misma operación sobre muchos datos al mismo tiempo.

**Terminología básica:**

| Término | Significado |
|---------|-------------|
| **Host** | La CPU y su memoria RAM |
| **Device** | La GPU y su memoria VRAM |
| **Kernel** | Función que se ejecuta en la GPU |
| **Thread** | Una unidad de ejecución dentro de la GPU |
| **Block** | Grupo de threads |
| **Grid** | Grupo de blocks |

---

## 2. Estructura de un Proyecto CUDA con CMake

Un proyecto CUDA separa el código en dos tipos de archivos:

- **`.cpp`** → Código que corre en CPU (host). Maneja memoria, UI, lógica general.
- **`.cu`** → Código que corre en GPU (device). Contiene los kernels.

### CMakeLists.txt mínimo

```cmake
cmake_minimum_required(VERSION 3.16)

project(MiProyecto)

enable_language(CXX CUDA)              # Habilitar compilador CUDA

find_package(fmt CONFIG REQUIRED)       # Dependencias

add_executable(mi_programa
    main.cpp                            # Código CPU
    kernel.cu                           # Código GPU
)

target_include_directories(mi_programa PRIVATE
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})  # Headers de CUDA

target_link_libraries(mi_programa PRIVATE fmt::fmt)
```

`enable_language(CXX CUDA)` es lo que permite compilar archivos `.cu`.

---

## 3. Calificadores de Función

Estos prefijos le dicen al compilador **dónde** se ejecuta cada función:

| Calificador | Se llama desde | Se ejecuta en |
|-------------|---------------|---------------|
| `__global__` | CPU (host) | GPU (device) |
| `__device__` | GPU (device) | GPU (device) |
| Sin prefijo | CPU (host) | CPU (host) |

### `__global__` — El Kernel

Es la función que la CPU lanza para que se ejecute en la GPU. Siempre retorna `void`.

```cpp
__global__ void sumaKernel(float* a, float* b, float* c, int n) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < n) {
        c[index] = a[index] + b[index];
    }
}
```

### `__device__` — Función auxiliar en GPU

Solo se puede llamar desde otro código GPU (`__global__` o `__device__`). Se usa para organizar lógica compleja dentro del kernel.

```cpp
__device__
uint32_t calcular_pixel(double x, double y, int max_iter) {
    // lógica que solo corre en GPU
    return color;
}
```

---

## 4. Modelo de Threads: Grid → Blocks → Threads

Cuando lanzas un kernel, especificas cuántos **blocks** y cuántos **threads por block** quieres:

```
kernel<<<num_blocks, threads_per_block>>>(args...);
```

### Variables internas del thread

Cada thread sabe su posición dentro del grid usando estas variables:

| Variable | Qué contiene |
|----------|-------------|
| `threadIdx.x` | Índice del thread dentro de su block |
| `blockIdx.x` | Índice del block dentro del grid |
| `blockDim.x` | Cantidad de threads por block |

### Fórmula del índice global

```cpp
int index = blockIdx.x * blockDim.x + threadIdx.x;
```

Ejemplo con `blockDim.x = 4`:

```
Block 0:  thread 0, 1, 2, 3     → index: 0, 1, 2, 3
Block 1:  thread 0, 1, 2, 3     → index: 4, 5, 6, 7
Block 2:  thread 0, 1, 2, 3     → index: 8, 9, 10, 11
```

### Guardia de límites

Siempre verificar que el index no se salga del rango de datos:

```cpp
if (index < n) {
    // operar sobre datos[index]
}
```

---

## 5. Cálculo de Blocks y Threads

Patrón estándar usado en todos los proyectos:

```cpp
int threads_per_block = 1024;  // Máximo típico por block
int num_blocks = (N + threads_per_block - 1) / threads_per_block;

miKernel<<<num_blocks, threads_per_block>>>(args...);
```

La fórmula `(N + T - 1) / T` es una **división entera redondeada hacia arriba**. Garantiza que se creen suficientes threads para cubrir todos los `N` elementos, aunque el último block quede parcialmente vacío.

---

## 6. Gestión de Memoria

### Flujo completo de memoria

```
1. Reservar en CPU (host)       → new / malloc
2. Reservar en GPU (device)     → cudaMalloc
3. Copiar CPU → GPU             → cudaMemcpy(..., HostToDevice)
4. Ejecutar kernel              → kernel<<<...>>>(...)
5. Sincronizar                  → cudaDeviceSynchronize()
6. Copiar GPU → CPU             → cudaMemcpy(..., DeviceToHost)
7. Liberar GPU                  → cudaFree
8. Liberar CPU                  → delete[] / free
```

### cudaMalloc — Reservar memoria en GPU

```cpp
float* d_A = nullptr;
size_t size_bytes = N * sizeof(float);

cudaMalloc((void**)&d_A, size_bytes);
```

- El puntero `d_A` apunta a memoria **en la GPU**.
- No se puede leer ni escribir directamente desde la CPU.
- Se debe calcular el tamaño en **bytes**.

### cudaMemcpy — Copiar datos entre CPU y GPU

```cpp
// CPU → GPU (subir datos)
cudaMemcpy(d_A, h_A, size_bytes, cudaMemcpyHostToDevice);

// GPU → CPU (bajar resultados)
cudaMemcpy(h_C, d_C, size_bytes, cudaMemcpyDeviceToHost);
```

| Dirección | Uso |
|-----------|-----|
| `cudaMemcpyHostToDevice` | Subir datos de CPU a GPU antes del kernel |
| `cudaMemcpyDeviceToHost` | Bajar resultados de GPU a CPU después del kernel |

### cudaFree — Liberar memoria en GPU

```cpp
cudaFree(d_A);
cudaFree(d_B);
cudaFree(d_C);
```

### cudaMemset — Inicializar memoria en GPU a un valor

```cpp
cudaMemset(d_hist, 0, NBINS * sizeof(int));
```

Pone todos los bytes a 0 en la memoria del device. Útil para limpiar buffers o histogramas antes de usarlos.

---

## 7. Sincronización

```cpp
cudaDeviceSynchronize();
```

Los kernels se ejecutan de forma **asíncrona**: la CPU lanza el kernel y sigue ejecutando su código sin esperar. `cudaDeviceSynchronize()` pausa la CPU hasta que la GPU termine.

**Cuándo usarlo:** Antes de leer resultados de la GPU con `cudaMemcpy`, o cuando se necesita medir tiempo.

---

## 8. Memoria `__constant__`

```cpp
__constant__ unsigned int d_color_ramp[PALETTE_SIZE];
```

Es memoria **de solo lectura** en la GPU que es rápida de acceder. Se usa para datos pequeños que todos los threads necesitan leer (como una paleta de colores).

### Copiar datos a memoria constante

```cpp
cudaMemcpyToSymbol(d_color_ramp, h_palette, PALETTE_SIZE * sizeof(unsigned int));
```

- `d_color_ramp` → variable `__constant__` destino en GPU.
- `h_palette` → datos fuente en CPU.
- Se copia una sola vez antes de usar los kernels.

---

## 9. Variables `__device__`

```cpp
__device__ static const double roots_r[3] = { 1.0, -0.5, -0.5 };
```

Arreglos que viven en la memoria global de la GPU. A diferencia de `__constant__`, se pueden tener más datos, pero el acceso es más lento. Útil para tablas de datos leídas dentro de funciones `__device__`.

---

## 10. `atomicAdd` — Operaciones atómicas

```cpp
atomicAdd(&d_hist[bin], 1);
```

Cuando muchos threads necesitan escribir en **la misma posición de memoria** al mismo tiempo (ej: acumular un histograma), ocurren conflictos. `atomicAdd` garantiza que la suma se haga de forma segura, un thread a la vez sobre esa dirección.

**Otros atomics disponibles:** `atomicSub`, `atomicMin`, `atomicMax`, `atomicExch`.

---

## 11. Consultar Propiedades del Dispositivo

```cpp
int deviceId = 0;
cudaSetDevice(deviceId);

cudaDeviceProp prop;
cudaGetDeviceProperties(&prop, deviceId);
```

Propiedades útiles:

| Propiedad | Qué es |
|-----------|--------|
| `prop.name` | Nombre de la GPU |
| `prop.totalGlobalMem` | Memoria total en bytes |
| `prop.multiProcessorCount` | Cantidad de SMs (multiprocesadores) |
| `prop.maxThreadsPerBlock` | Máximo de threads por block (típicamente 1024) |
| `prop.maxThreadsPerMultiProcessor` | Máximo de threads por SM |
| `prop.maxGridSize[0/1/2]` | Tamaño máximo del grid en cada dimensión |

---

## 12. Macro CHECK para Errores CUDA

```cpp
#define CHECK(expr) {                               \
        auto err = (expr);                          \
        if (err != cudaSuccess) {                   \
            fmt::println("CUDA Error {}: {} at line {}", \
                (int)err, cudaGetErrorString(err), __LINE__); \
            exit(EXIT_FAILURE);                     \
        }                                           \
    }
```

**Uso:**

```cpp
CHECK(cudaMalloc(&d_ptr, size));
CHECK(cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost));
CHECK(cudaGetLastError());  // Verifica error del último kernel
```

`cudaGetLastError()` atrapa errores de lanzamiento del kernel (parámetros inválidos, grid demasiado grande, etc.).

---

## 13. Patrón de Comunicación CPU ↔ GPU (extern "C")

Cuando el kernel está en `kernel.cu` y el `main` está en `main.cpp`, se necesita una **función wrapper** para conectarlos:

### En kernel.cu — Definir el wrapper

```cpp
// El kernel (no se puede llamar desde .cpp directamente)
__global__ void miKernel(float* a, float* b, float* c, int n) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < n) {
        c[index] = a[index] + b[index];
    }
}

// Wrapper que se puede llamar desde .cpp
extern "C" void lanzar_kernel(float* a, float* b, float* c, int n) {
    int threads = 1024;
    int blocks = (n + threads - 1) / threads;
    miKernel<<<blocks, threads>>>(a, b, c, n);
}
```

### En main.cpp — Declarar y usar

```cpp
extern "C" void lanzar_kernel(float* a, float* b, float* c, int n);

int main() {
    // ... reservar memoria ...
    lanzar_kernel(d_A, d_B, d_C, N);
    cudaDeviceSynchronize();
    // ... copiar resultados ...
}
```

`extern "C"` evita que C++ decore el nombre de la función, permitiendo que el linker la encuentre entre archivos `.cu` y `.cpp`.

---

## 14. Mapeo de Datos 2D a Índice 1D

Para imágenes o matrices (2D) procesadas con threads (1D):

```cpp
int index = blockIdx.x * blockDim.x + threadIdx.x;

if (index < width * height) {
    uint32_t col = index % width;    // Columna (x)
    uint32_t row = index / width;    // Fila (y)

    // Usar col y row para calcular coordenadas
    pixel_buffer[index] = calcular(col, row);
}
```

Esto convierte un índice lineal (`0, 1, 2, ...`) en coordenadas 2D `(col, row)`.

---

## 15. Ejemplo Completo Mínimo: Suma de Vectores

### kernel.cu

```cpp
#include <cuda_runtime.h>
#include <cmath>

__global__ void sumaKernel(float* a, float* b, float* c, int n) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < n) {
        c[index] = a[index] + b[index];
    }
}

void sumaVectores(float* a, float* b, float* c, int n) {
    int threads_per_block = 1024;
    int num_blocks = (int)ceil(n * 1.0 / threads_per_block);
    sumaKernel<<<num_blocks, threads_per_block>>>(a, b, c, n);
}
```

### main.cpp

```cpp
#include <cuda_runtime.h>

extern void sumaVectores(float* a, float* b, float* c, int n);

int main() {
    const int N = 1024 * 1024;

    // 1. Reservar memoria en CPU
    float* h_A = new float[N];
    float* h_B = new float[N];
    float* h_C = new float[N];

    for (int i = 0; i < N; i++) {
        h_A[i] = 1.0f;
        h_B[i] = 2.0f;
    }

    // 2. Reservar memoria en GPU
    float *d_A, *d_B, *d_C;
    size_t bytes = N * sizeof(float);
    cudaMalloc((void**)&d_A, bytes);
    cudaMalloc((void**)&d_B, bytes);
    cudaMalloc((void**)&d_C, bytes);

    // 3. Copiar datos CPU → GPU
    cudaMemcpy(d_A, h_A, bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, bytes, cudaMemcpyHostToDevice);

    // 4. Ejecutar kernel
    sumaVectores(d_A, d_B, d_C, N);

    // 5. Esperar a que la GPU termine
    cudaDeviceSynchronize();

    // 6. Copiar resultados GPU → CPU
    cudaMemcpy(h_C, d_C, bytes, cudaMemcpyDeviceToHost);

    // 7. Liberar memoria GPU
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    // 8. Liberar memoria CPU
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;

    return 0;
}
```

---

## 16. Ejemplo Completo Avanzado: Fractal en GPU

### kernel.cu

```cpp
#include <cuda_runtime.h>
#include <cstdint>
#include "palette.h"

__constant__ unsigned int d_color_ramp[PALETTE_SIZE];

__device__
uint32_t calcular_pixel(double x, double y, double cr, double ci, int max_iter) {
    int iter = 0;
    double zr = x, zi = y;

    while (iter < max_iter && (zr * zr + zi * zi) < 4.0) {
        double dr = zr * zr - zi * zi + cr;
        double di = 2.0 * zr * zi + ci;
        zr = dr;
        zi = di;
        iter++;
    }

    if (iter < max_iter) {
        return d_color_ramp[iter % PALETTE_SIZE];
    }
    return 0x000000FF;
}

__global__
void fractal_kernel(double cr, double ci, int max_iter,
                    double x_min, double y_min, double x_max, double y_max,
                    uint32_t width, uint32_t height, uint32_t* pixels) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < width * height) {
        uint32_t col = index % width;
        uint32_t row = index / width;

        double x = x_min + col * (x_max - x_min) / width;
        double y = y_max - row * (y_max - y_min) / height;

        pixels[index] = calcular_pixel(x, y, cr, ci, max_iter);
    }
}

extern "C" void copiar_paleta(unsigned int* h_palette) {
    cudaMemcpyToSymbol(d_color_ramp, h_palette, PALETTE_SIZE * sizeof(unsigned int));
}

extern "C" void fractal_gpu(double cr, double ci, int max_iter,
                            double x_min, double y_min, double x_max, double y_max,
                            uint32_t w, uint32_t h, uint32_t* pixels) {
    int threads = 1024;
    int blocks = (w * h + threads - 1) / threads;
    fractal_kernel<<<blocks, threads>>>(cr, ci, max_iter,
                                        x_min, y_min, x_max, y_max,
                                        w, h, pixels);
}
```

### main.cpp (flujo GPU relevante)

```cpp
#include <cuda_runtime.h>
#include "palette.h"

extern "C" void copiar_paleta(unsigned int* h_palette);
extern "C" void fractal_gpu(double cr, double ci, int max_iter,
                            double x_min, double y_min, double x_max, double y_max,
                            uint32_t w, uint32_t h, uint32_t* pixels);

int main() {
    const uint32_t W = 1920, H = 1080;
    size_t buffer_size = W * H * sizeof(uint32_t);

    // Buffer en CPU
    uint32_t* host_pixels = (uint32_t*)malloc(buffer_size);

    // Buffer en GPU
    uint32_t* device_pixels;
    cudaMalloc(&device_pixels, buffer_size);

    // Copiar paleta a memoria __constant__
    copiar_paleta(color_ramp.data());

    // En cada frame:
    fractal_gpu(cr, ci, max_iter, x_min, y_min, x_max, y_max, W, H, device_pixels);
    cudaMemcpy(host_pixels, device_pixels, buffer_size, cudaMemcpyDeviceToHost);

    // Liberar
    free(host_pixels);
    cudaFree(device_pixels);
}
```

---

## 17. Proyecto Híbrido: CUDA + MPI + OpenMP

El proyecto híbrido divide la imagen en franjas horizontales y asigna una franja a cada proceso MPI. Cada proceso puede elegir ejecutar su franja con Serial, OpenMP, o CUDA.

### División de trabajo

```cpp
int delta = ceil(HEIGHT * 1.0 / nprocs);  // Filas por proceso
int row_start = rank * delta;
int row_end   = row_start + delta;
if (row_end > HEIGHT) row_end = HEIGHT;
```

### Kernel con rango de filas

```cpp
__global__
void newton_kernel(int max_iter,
                   double x_min, double y_min, double x_max, double y_max,
                   uint32_t width, uint32_t height,
                   uint32_t row_start, uint32_t row_end,
                   uint32_t* pixel_buffer, unsigned long long* d_iters)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t num_pixels = width * (row_end - row_start);

    if (idx < num_pixels) {
        uint32_t col = idx % width;
        uint32_t row = row_start + (idx / width);
        // ... calcular pixel ...
        atomicAdd(d_iters, (unsigned long long)iter);
    }
}
```

### Asignación de GPU por rank

```cpp
int dev_count = 0;
cudaGetDeviceCount(&dev_count);
if (dev_count > 0) {
    cudaSetDevice(rank % dev_count);
}
```

`cudaGetDeviceCount` obtiene cuántas GPUs hay. `rank % dev_count` distribuye los procesos MPI entre las GPUs disponibles.

---

## 18. Resumen Rápido de la API CUDA

| Función | Qué hace |
|---------|----------|
| `cudaSetDevice(id)` | Seleccionar qué GPU usar |
| `cudaGetDeviceProperties(&prop, id)` | Obtener info de la GPU |
| `cudaGetDeviceCount(&count)` | Cantidad de GPUs disponibles |
| `cudaMalloc(&ptr, bytes)` | Reservar memoria en GPU |
| `cudaFree(ptr)` | Liberar memoria en GPU |
| `cudaMemcpy(dst, src, bytes, dir)` | Copiar datos entre CPU y GPU |
| `cudaMemcpyToSymbol(sym, src, bytes)` | Copiar datos a memoria `__constant__` |
| `cudaMemset(ptr, val, bytes)` | Inicializar memoria en GPU |
| `cudaDeviceSynchronize()` | Esperar que la GPU termine |
| `cudaGetLastError()` | Obtener el último error de CUDA |
| `cudaGetErrorString(err)` | Convertir código de error a texto |
| `kernel<<<blocks, threads>>>(...)` | Lanzar un kernel |

---

## 19. Errores Comunes

| Error | Causa | Solución |
|-------|-------|----------|
| Resultados basura | Faltó `cudaDeviceSynchronize()` antes de `cudaMemcpy` DeviceToHost | Sincronizar antes de leer |
| Segfault en GPU | `index` fuera de rango | Agregar `if (index < N)` |
| Kernel no ejecuta | Demasiados threads por block | Usar máximo 1024 |
| Datos no llegan a GPU | Olvidar `cudaMemcpy` HostToDevice | Copiar antes de lanzar kernel |
| Función __device__ no compila en .cpp | `__device__` solo existe en `.cu` | Mover a archivo `.cu` |
| Linker no encuentra el wrapper | Falta `extern "C"` | Agregar en declaración y definición |

---

## 20. Checklist para un Proyecto CUDA Nuevo

```
□ Crear CMakeLists.txt con enable_language(CXX CUDA)
□ Separar código: main.cpp (host) + kernel.cu (device)
□ Definir el kernel con __global__
□ Calcular index = blockIdx.x * blockDim.x + threadIdx.x
□ Agregar guardia if (index < N)
□ Crear función wrapper con extern "C" en kernel.cu
□ Declarar el wrapper con extern "C" en main.cpp
□ cudaMalloc para cada buffer en GPU
□ cudaMemcpy HostToDevice para datos de entrada
□ Lanzar kernel<<<blocks, threads>>>(args)
□ cudaDeviceSynchronize()
□ cudaMemcpy DeviceToHost para resultados
□ cudaFree para cada buffer
□ Liberar memoria del host
```
