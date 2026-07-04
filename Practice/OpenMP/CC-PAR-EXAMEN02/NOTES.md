# NOTES.md - Plantilla universal para proyectos C++ de computacion paralela

Este documento sirve como guia base para proyectos tipo examen/laboratorio en C++ donde se trabaja con procesamiento de datos, imagenes, memoria, CMake, paralelismo con OpenMP y optimizacion con SIMD.

## 1. Plantilla universal del proyecto

### 1.1. Estructura recomendada

```text
proyecto/
├── CMakeLists.txt
├── main.cpp
├── modulo.h
├── modulo.cpp
├── assets/
│   └── entrada.jpg
├── build/
└── NOTES.md
```

En proyectos pequenos tambien es valido tener los recursos en la raiz, como `img.jpg`, pero en proyectos grandes conviene separarlos en `assets/`.

### 1.2. Responsabilidad de cada archivo

| Archivo | Proposito |
|---|---|
| `CMakeLists.txt` | Define como compilar, que estandar usar, que librerias enlazar y que flags activar. |
| `main.cpp` | Punto de entrada. Carga datos, llama funciones, mide tiempos y muestra resultados. |
| `.h` / `.hpp` | Declaraciones: funciones, estructuras, clases, constantes publicas. |
| `.cpp` | Implementaciones: logica real de las funciones declaradas en headers. |
| `build/` | Carpeta generada por CMake. No se edita manualmente. |

### 1.3. Plantilla base de CMake

```cmake
cmake_minimum_required(VERSION 3.16)

project(NombreProyecto)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NombreProyecto
    main.cpp
    modulo.cpp
)
```

Para OpenMP:

```cmake
find_package(OpenMP REQUIRED)
target_link_libraries(NombreProyecto PRIVATE OpenMP::OpenMP_CXX)
```

Para AVX2/SIMD con GCC/MinGW:

```cmake
target_compile_options(NombreProyecto PRIVATE -mavx2)
```

Para SFML:

```cmake
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)
target_link_libraries(NombreProyecto PRIVATE
    SFML::System
    SFML::Graphics
    SFML::Window
)
```

### 1.4. Flujo tipico de un proyecto de imagenes

1. Cargar imagen desde disco.
2. Obtener ancho, alto y numero de canales.
3. Reservar memoria para la salida.
4. Procesar cada pixel.
5. Medir tiempo.
6. Mostrar o guardar resultado.
7. Liberar memoria.

Ejemplo conceptual:

```cpp
int width, height, channels;
uint8_t* input = cargar_imagen("img.jpg", &width, &height, &channels);

uint32_t total_pixels = width * height;
uint8_t* output = new uint8_t[total_pixels];

procesar(input, output, width, height);

delete[] output;
liberar_imagen(input);
```

## 2. Sintaxis esencial de C++ y comparacion con Java

### 2.1. Funcion principal

C++:

```cpp
int main() {
    return 0;
}
```

Java:

```java
public static void main(String[] args) {
}
```

Comparacion:

| Concepto | C++ | Java |
|---|---|---|
| Punto de entrada | `int main()` | `public static void main(String[] args)` |
| Valor de salida | `return 0` indica exito | Normalmente no retorna valor |
| Archivo obligatorio | No necesita clase contenedora | Todo debe estar dentro de una clase |

### 2.2. Includes vs imports

C++:

```cpp
#include <iostream>
#include "filtro.h"
```

Java:

```java
import java.util.Scanner;
```

Comparacion:

| C++ | Java |
|---|---|
| `#include` copia declaraciones antes de compilar. | `import` referencia clases de paquetes. |
| Usa headers `.h` / `.hpp`. | Usa paquetes y clases. |
| Puede incluir librerias del sistema con `<...>` o locales con `"..."`. | Usa rutas logicas de paquete. |

### 2.3. Variables y tipos

C++:

```cpp
int edad = 20;
float peso = 70.5f;
double promedio = 9.75;
char letra = 'A';
bool activo = true;
```

Java:

```java
int edad = 20;
float peso = 70.5f;
double promedio = 9.75;
char letra = 'A';
boolean activo = true;
```

Tipos importantes en C++:

| Tipo | Uso |
|---|---|
| `int` | Enteros normales. |
| `uint8_t` | Entero sin signo de 8 bits. Muy usado para pixeles. |
| `uint32_t` | Entero sin signo de 32 bits. Util para indices grandes. |
| `float` | Decimal de 32 bits. |
| `double` | Decimal de 64 bits, mas precision. |
| `size_t` | Tipo recomendado para tamanos de arreglos y contenedores. |

### 2.4. Constantes

C++:

```cpp
const float WR = 0.21f;
constexpr int CANALES_RGBA = 4;
```

Java:

```java
final float WR = 0.21f;
static final int CANALES_RGBA = 4;
```

Comparacion:

| C++ | Java |
|---|---|
| `const` evita modificar una variable. | `final` evita reasignar una variable. |
| `constexpr` permite calcular en tiempo de compilacion. | `static final` se usa para constantes de clase. |

### 2.5. Condicionales

C++ y Java son muy similares:

```cpp
if (x > 0) {
    // positivo
} else if (x < 0) {
    // negativo
} else {
    // cero
}
```

### 2.6. Switch

C++:

```cpp
switch (opcion) {
    case 1:
        break;
    case 2:
        break;
    default:
        break;
}
```

Java es similar. En ambos lenguajes, si falta `break`, la ejecucion puede continuar al siguiente caso.

### 2.7. Bucles

C++:

```cpp
for (int i = 0; i < n; ++i) {
}

while (condicion) {
}

do {
} while (condicion);
```

Java es casi igual.

Detalle importante:

```cpp
++i;
i++;
```

En tipos primitivos casi no hay diferencia. En iteradores u objetos, `++i` puede ser mas eficiente porque evita una copia temporal.

### 2.8. Funciones

C++:

```cpp
int sumar(int a, int b) {
    return a + b;
}
```

Java:

```java
static int sumar(int a, int b) {
    return a + b;
}
```

Comparacion:

| C++ | Java |
|---|---|
| Puede tener funciones libres fuera de clases. | Las funciones deben estar dentro de clases. |
| Se declaran en `.h` y se implementan en `.cpp`. | Se escriben como metodos. |

### 2.9. Paso por valor, referencia y puntero

C++ por valor:

```cpp
void f(int x) {
    x = 10;
}
```

No modifica la variable original.

C++ por referencia:

```cpp
void f(int& x) {
    x = 10;
}
```

Si modifica la variable original.

C++ por puntero:

```cpp
void f(int* x) {
    *x = 10;
}
```

Tambien modifica la variable original, pero puede recibir `nullptr`.

Java:

```java
void f(Objeto obj) {
    obj.valor = 10;
}
```

Java siempre pasa argumentos por valor. Cuando se pasa un objeto, se copia el valor de la referencia, no el objeto completo.

### 2.10. Punteros

C++:

```cpp
int x = 5;
int* p = &x;
*p = 10;
```

Significado:

| Sintaxis | Significado |
|---|---|
| `int* p` | `p` es un puntero a entero. |
| `&x` | Direccion de memoria de `x`. |
| `*p` | Valor almacenado en la direccion apuntada por `p`. |
| `nullptr` | Puntero nulo moderno. |

Java no tiene punteros explicitos. Tiene referencias administradas por la JVM.

### 2.11. Arreglos dinamicos

C++:

```cpp
int* datos = new int[n];
delete[] datos;
```

Java:

```java
int[] datos = new int[n];
```

Comparacion:

| C++ | Java |
|---|---|
| El programador puede reservar y liberar memoria manualmente. | El recolector de basura libera memoria automaticamente. |
| Si olvidas `delete[]`, hay fuga de memoria. | Si no hay referencias al arreglo, la JVM lo limpia luego. |
| Acceso fuera de rango puede causar comportamiento indefinido. | Acceso fuera de rango lanza `ArrayIndexOutOfBoundsException`. |

Preferencia moderna en C++:

```cpp
#include <vector>

std::vector<int> datos(n);
```

`std::vector` libera memoria automaticamente cuando sale de alcance.

### 2.12. Structs y clases

C++:

```cpp
struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};
```

Java:

```java
class Pixel {
    int r;
    int g;
    int b;
    int a;
}
```

Comparacion:

| C++ | Java |
|---|---|
| `struct` tiene miembros publicos por defecto. | No existe `struct`. |
| `class` tiene miembros privados por defecto. | `class` es la unidad normal de modelado. |
| Puede crear objetos en stack o heap. | Los objetos se crean en heap con `new`. |

### 2.13. Namespaces

C++:

```cpp
std::cout << "Hola";
```

`std` es un namespace. Evita choques de nombres.

Java usa paquetes:

```java
java.util.List
```

### 2.14. Casting

C++:

```cpp
float x = static_cast<float>(entero);
uint8_t g = static_cast<uint8_t>(valor);
```

Java:

```java
float x = (float) entero;
byte g = (byte) valor;
```

En C++ moderno se prefiere `static_cast<T>(valor)` porque expresa mejor la intencion.

### 2.15. Enums

C++:

```cpp
enum class Modo {
    ORIGINAL,
    FILTRO
};
```

Java:

```java
enum Modo {
    ORIGINAL,
    FILTRO
}
```

`enum class` en C++ es mas seguro que `enum` tradicional porque evita conversiones implicitas a entero.

### 2.16. Headers y guards

C++:

```cpp
#ifndef FILTRO_H
#define FILTRO_H

void procesar();

#endif
```

Sirve para evitar que un header se incluya multiples veces. Alternativa comun:

```cpp
#pragma once
```

Java no necesita headers.

## 3. Memoria en C++

### 3.1. Stack y heap

| Zona | Caracteristicas | Ejemplo |
|---|---|---|
| Stack | Rapida, automatica, limitada. | `int x = 5;` |
| Heap | Mas grande, manual o administrada por contenedores. | `new int[n]`, `std::vector<int>` |

Ejemplo stack:

```cpp
int x = 10;
```

Ejemplo heap manual:

```cpp
int* datos = new int[100];
delete[] datos;
```

Ejemplo heap recomendado:

```cpp
std::vector<int> datos(100);
```

### 3.2. Errores comunes de memoria

| Error | Descripcion |
|---|---|
| Memory leak | Reservar memoria y no liberarla. |
| Dangling pointer | Usar un puntero despues de liberar la memoria. |
| Buffer overflow | Escribir fuera del arreglo. |
| Use after free | Acceder memoria ya liberada. |
| Double free | Liberar dos veces el mismo bloque. |

### 3.3. RAII

RAII significa "Resource Acquisition Is Initialization". La idea es que los recursos se liberen automaticamente cuando el objeto sale de alcance.

Ejemplo recomendado:

```cpp
std::vector<uint8_t> gray_pixels(total_pixels);
```

En vez de:

```cpp
uint8_t* gray_pixels = new uint8_t[total_pixels];
delete[] gray_pixels;
```

### 3.4. Imagenes en memoria

Una imagen RGBA suele almacenarse como un arreglo lineal:

```text
R G B A R G B A R G B A ...
```

Para acceder al pixel `i`:

```cpp
uint32_t idx = i * 4;
uint8_t r = rgba_pixels[idx];
uint8_t g = rgba_pixels[idx + 1];
uint8_t b = rgba_pixels[idx + 2];
uint8_t a = rgba_pixels[idx + 3];
```

Para coordenadas `(x, y)`:

```cpp
uint32_t i = y * width + x;
uint32_t idx = i * 4;
```

## 4. Conceptos matematicos necesarios

### 4.1. Imagen como matriz

Una imagen de ancho `width` y alto `height` puede verse como una matriz:

```text
height filas x width columnas
```

Pero en memoria normalmente esta linealizada:

```cpp
indice = y * width + x;
```

### 4.2. Canales de color

Formatos comunes:

| Formato | Canales | Significado |
|---|---:|---|
| RGB | 3 | Rojo, verde, azul. |
| RGBA | 4 | Rojo, verde, azul, alfa/transparencia. |
| Grayscale | 1 | Intensidad de gris. |

Cada canal normalmente va de `0` a `255`.

### 4.3. Conversion a escala de grises

Formula perceptual comun:

```text
gray = 0.21R + 0.72G + 0.07B
```

Razon:

| Canal | Peso | Motivo |
|---|---:|---|
| Verde | 0.72 | El ojo humano es mas sensible al verde. |
| Rojo | 0.21 | Sensibilidad media. |
| Azul | 0.07 | Menor sensibilidad. |

En C++:

```cpp
gray_pixels[i] = static_cast<uint8_t>(0.21f * r + 0.72f * g + 0.07f * b);
```

### 4.4. Division de trabajo

Si hay `N` pixeles y `T` hilos:

```text
chunk = ceil(N / T)
start = thread_id * chunk
end = min(start + chunk, N)
```

Esto divide el arreglo en regiones.

Ejemplo:

```cpp
int delta = std::ceil(total_pixels * 1.0 / thread_count);
int start = thread_id * delta;
int end = std::min(start + delta, total_pixels);
```

### 4.5. Complejidad algoritmica

Para procesar todos los pixeles una vez:

```text
O(width * height)
```

Si la imagen tiene `N` pixeles:

```text
O(N)
```

Esto significa que si duplicas la cantidad de pixeles, aproximadamente duplicas el trabajo.

## 5. Paralelismo con OpenMP

### 5.1. Idea principal

OpenMP permite ejecutar partes del programa en varios hilos con directivas `#pragma`.

Ejemplo:

```cpp
#pragma omp parallel for
for (int i = 0; i < n; ++i) {
    salida[i] = entrada[i] * 2;
}
```

### 5.2. `parallel` vs `parallel for`

```cpp
#pragma omp parallel
{
    int id = omp_get_thread_num();
}
```

Crea una region paralela. Cada hilo ejecuta el bloque completo.

```cpp
#pragma omp parallel for
for (int i = 0; i < n; ++i) {
}
```

Divide automaticamente las iteraciones del `for` entre los hilos.

### 5.3. Funciones importantes

| Funcion | Significado |
|---|---|
| `omp_get_thread_num()` | ID del hilo actual. |
| `omp_get_num_threads()` | Cantidad de hilos dentro de la region paralela. |
| `omp_get_max_threads()` | Maximo de hilos disponibles. |
| `omp_get_wtime()` | Tiempo en segundos para medir rendimiento. |

### 5.4. Variables privadas y compartidas

En OpenMP:

| Tipo | Significado |
|---|---|
| Compartida | Todos los hilos ven la misma variable. |
| Privada | Cada hilo tiene su propia copia. |

Ejemplo:

```cpp
#pragma omp parallel for
for (int i = 0; i < n; ++i) {
    int local = i * 2;
    salida[i] = local;
}
```

`i` y `local` son privados por iteracion. `salida` es compartida.

### 5.5. Condiciones de carrera

Una condicion de carrera ocurre cuando varios hilos modifican el mismo dato al mismo tiempo.

Ejemplo peligroso:

```cpp
int suma = 0;

#pragma omp parallel for
for (int i = 0; i < n; ++i) {
    suma += datos[i];
}
```

Solucion con reduccion:

```cpp
int suma = 0;

#pragma omp parallel for reduction(+:suma)
for (int i = 0; i < n; ++i) {
    suma += datos[i];
}
```

### 5.6. Speedup y eficiencia

Speedup:

```text
S = tiempo_secuencial / tiempo_paralelo
```

Eficiencia:

```text
E = S / numero_hilos
```

Ejemplo:

```text
tiempo secuencial = 100 ms
tiempo paralelo = 25 ms
S = 100 / 25 = 4
```

Con 8 hilos:

```text
E = 4 / 8 = 0.5 = 50%
```

## 6. SIMD y AVX2

### 6.1. Que es SIMD

SIMD significa "Single Instruction, Multiple Data". Permite aplicar una misma operacion a varios datos al mismo tiempo.

Ejemplo conceptual:

```text
Normal:
gray[0] = f(pixel[0])
gray[1] = f(pixel[1])
gray[2] = f(pixel[2])

SIMD:
gray[0..7] = f(pixel[0..7])
```

### 6.2. Tipos AVX comunes

| Tipo | Contenido |
|---|---|
| `__m128` | 4 floats de 32 bits. |
| `__m256` | 8 floats de 32 bits. |
| `__m256i` | Enteros empaquetados en 256 bits. |

### 6.3. Intrinsics frecuentes

| Intrinsic | Uso |
|---|---|
| `_mm256_set1_ps(x)` | Llena los 8 floats con el mismo valor. |
| `_mm256_loadu_ps(ptr)` | Carga 8 floats desde memoria no necesariamente alineada. |
| `_mm256_storeu_ps(ptr, v)` | Guarda 8 floats en memoria. |
| `_mm256_mul_ps(a, b)` | Multiplica 8 floats. |
| `_mm256_add_ps(a, b)` | Suma 8 floats. |

### 6.4. Cuidado con restos

Si procesas de 8 en 8:

```cpp
for (uint32_t i = 0; i + 7 < total_pixels; i += 8) {
    // procesa 8 pixeles
}
```

Luego debes procesar los pixeles restantes:

```cpp
for (; i < total_pixels; ++i) {
    // procesa 1 pixel
}
```

Si no lo haces, puedes leer fuera del arreglo cuando `total_pixels` no es multiplo de 8.

### 6.5. Alineacion de memoria

SIMD puede ser mas rapido si los datos estan alineados, pero las funciones `loadu` y `storeu` permiten memoria no alineada.

| Funcion | Requiere alineacion |
|---|---|
| `_mm256_load_ps` | Si |
| `_mm256_loadu_ps` | No |
| `_mm256_store_ps` | Si |
| `_mm256_storeu_ps` | No |

## 7. Medicion de tiempo

### 7.1. Con `std::chrono`

```cpp
auto start = std::chrono::high_resolution_clock::now();

procesar();

auto end = std::chrono::high_resolution_clock::now();
double ms = std::chrono::duration<double, std::milli>(end - start).count();
```

### 7.2. Con OpenMP

```cpp
double start = omp_get_wtime();

procesar();

double end = omp_get_wtime();
double ms = (end - start) * 1000.0;
```

### 7.3. Recomendaciones para benchmarks

1. Ejecutar varias veces y promediar.
2. Ignorar la primera ejecucion si hay carga inicial.
3. Medir solo la parte que interesa, no la carga de archivo ni la ventana grafica.
4. Usar la misma imagen y la misma configuracion para comparar.
5. Compilar en modo Release.

## 8. Errores frecuentes en proyectos como este

### 8.1. Indices incorrectos en imagen RGBA

Correcto:

```cpp
uint32_t idx = i * 4;
r = rgba_pixels[idx];
g = rgba_pixels[idx + 1];
b = rgba_pixels[idx + 2];
```

Incorrecto:

```cpp
r = rgba_pixels[i];
g = rgba_pixels[i + 1];
b = rgba_pixels[i + 2];
```

El segundo ejemplo ignora que cada pixel ocupa 4 bytes en RGBA.

### 8.2. Escribir en la variable equivocada

Correcto:

```cpp
r_arr[p] = rgba_pixels[idx];
g_arr[p] = rgba_pixels[idx + 1];
b_arr[p] = rgba_pixels[idx + 2];
```

Incorrecto:

```cpp
r_arr[p] = rgba_pixels[idx];
r_arr[p] = rgba_pixels[idx + 1];
r_arr[p] = rgba_pixels[idx + 2];
```

El segundo ejemplo sobrescribe `r_arr[p]` tres veces y deja `g_arr` y `b_arr` sin inicializar.

### 8.3. No controlar limites en SIMD

Peligroso:

```cpp
for (uint32_t i = 0; i < total_pixels; i += 8) {
    // puede pasarse si total_pixels no es multiplo de 8
}
```

Mas seguro:

```cpp
uint32_t i = 0;
for (; i + 7 < total_pixels; i += 8) {
    // procesa 8
}
for (; i < total_pixels; ++i) {
    // procesa resto
}
```

### 8.4. Division de regiones con desbordes

Siempre conviene limitar el final:

```cpp
int end = std::min(start + delta, static_cast<int>(total_pixels));
```

### 8.5. No liberar memoria

Si se usa `new[]`, debe existir `delete[]`.

```cpp
uint8_t* buffer = new uint8_t[n];
delete[] buffer;
```

Mejor:

```cpp
std::vector<uint8_t> buffer(n);
```

## 9. Checklist para entregar un proyecto

Antes de entregar:

1. El proyecto compila desde cero.
2. `CMakeLists.txt` incluye todas las fuentes.
3. No hay rutas absolutas innecesarias.
4. Los archivos de entrada estan incluidos o documentados.
5. Se controla si la imagen no carga.
6. Se libera memoria o se usan contenedores RAII.
7. Los bucles no leen fuera de rango.
8. Las funciones tienen nombres claros.
9. Los resultados se pueden guardar o visualizar.
10. Se reporta tiempo de ejecucion si el objetivo es rendimiento.
11. Se comparan versiones secuencial, paralela o SIMD cuando aplique.
12. Se documentan teclas, comandos o parametros de ejecucion.

## 10. Plantilla de explicacion para informe

Puedes usar esta estructura para explicar el proyecto:

```text
1. Objetivo
   Implementar un filtro de escala de grises sobre una imagen usando C++.

2. Entrada
   Imagen en formato JPG cargada como arreglo RGBA.

3. Proceso
   Cada pixel se transforma usando:
   gray = 0.21R + 0.72G + 0.07B

4. Versiones implementadas
   - Version SIMD con AVX2.
   - Version paralela por regiones con OpenMP.

5. Salida
   Imagen en escala de grises mostrada en pantalla o guardada como PNG.

6. Analisis
   La complejidad es O(width * height). El paralelismo reduce el tiempo al dividir los pixeles entre hilos.

7. Riesgos tecnicos
   - Accesos fuera de rango.
   - Mala division de pixeles.
   - Condiciones de carrera.
   - Memoria no liberada.
```

## 11. Glosario rapido

| Termino | Definicion |
|---|---|
| Pixel | Unidad minima de una imagen. |
| Canal | Componente de color: R, G, B o A. |
| RGBA | Formato con rojo, verde, azul y alfa. |
| Grayscale | Imagen de un solo canal de intensidad. |
| Buffer | Bloque de memoria usado para guardar datos. |
| Header | Archivo `.h` con declaraciones. |
| Compilador | Programa que traduce C++ a codigo ejecutable. |
| Linker | Une objetos compilados y librerias. |
| SIMD | Una instruccion opera sobre multiples datos. |
| OpenMP | API para paralelismo con directivas. |
| Thread | Hilo de ejecucion. |
| Race condition | Error por acceso concurrente no controlado. |
| Speedup | Cuanto mas rapido es un programa paralelo. |
| Heap | Memoria dinamica. |
| Stack | Memoria automatica local. |
| RAII | Patron para liberar recursos automaticamente. |

## 12. Mini chuleta C++ vs Java

| Tema | C++ | Java |
|---|---|---|
| Archivo principal | Puede ser `main.cpp`. | Clase con `main`. |
| Librerias | `#include`. | `import`. |
| Memoria | Manual o RAII. | Garbage collector. |
| Punteros | Si, explicitos. | No explicitos. |
| Referencias | `int&`. | Referencias a objetos administradas. |
| Funciones libres | Si. | No, todo en clases. |
| Compilacion | Compila a binario nativo. | Compila a bytecode para JVM. |
| Rendimiento bajo nivel | Muy alto control. | Menos control directo. |
| SIMD | Intrinsics como AVX2. | No es comun usar intrinsics directamente. |
| Paralelismo simple | OpenMP, threads, async. | Threads, executors, streams paralelos. |
| Arreglos fuera de rango | Comportamiento indefinido. | Excepcion. |
| Genericos | Templates. | Generics. |
| Constantes | `const`, `constexpr`. | `final`, `static final`. |
| Enumeraciones | `enum class`. | `enum`. |

