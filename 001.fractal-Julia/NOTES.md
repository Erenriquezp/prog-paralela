# NOTAS DE PRÁCTICA — Fractal de Julia con SIMD AVX2

**Asignatura:** Programación Paralela  
**Proyecto:** `FractalJulia`  
**Fecha:** Abril 2026  
**Lenguaje:** C++17  
**Sistema operativo:** Windows 64-bit

---

## Tabla de contenidos

1. [Herramientas y entorno](#1-herramientas-y-entorno)
2. [Arquitectura del proyecto](#2-arquitectura-del-proyecto)
3. [Chuleta de sintaxis C++](#3-chuleta-de-sintaxis-c)
4. [Algoritmo del conjunto de Julia](#4-algoritmo-del-conjunto-de-julia)
5. [Comparación de implementaciones](#5-comparación-de-implementaciones)
6. [Observaciones técnicas y límites](#6-observaciones-técnicas-y-límites)
7. [Glosario](#7-glosario)

---

## 1. Herramientas y entorno

### 1.1 Compilador — MinGW-w64 GCC/G++

| Aspecto             | Detalle                                                 |
| ------------------- | ------------------------------------------------------- |
| Ruta                | `c:/tools/mingw64/bin/g++.exe`                          |
| Estándar habilitado | C++17 (por defecto en versiones recientes de MinGW-w64) |
| Flag clave          | `-mavx2` — habilita instrucciones AVX2 para SIMD        |
| Uso directo         | `g++ -std=c++17 -mavx2 -O2 archivo.cpp -o programa`     |

El flag `-mavx2` se declara en `CMakeLists.txt` como `CMAKE_CXX_FLAGS` para que aplique a todas las unidades de compilación del target.

### 1.2 Sistema de build — CMake + Ninja Multi-Config

**CMake** genera los archivos de build a partir de `CMakeLists.txt`. En este proyecto se usa el generador **Ninja Multi-Config**, que permite mantener simultáneamente varias configuraciones (Debug, Release, RelWithDebInfo) dentro del mismo directorio `build/`, cada una en su propia subcarpeta.

```
build/
  Debug/          <-- ejecutable con símbolos, sin optimización
  Release/        <-- ejecutable optimizado
  RelWithDebInfo/ <-- optimizado con símbolos para profiling
  build.ninja     <-- archivo maestro de Ninja
  CMakeCache.txt  <-- caché de variables de CMake
```

La selección de generador y directorio de build se configura en `.vscode/settings.json`:

```json
"cmake.generator": "Ninja Multi-Config",
"cmake.buildDirectory": "${workspaceFolder}/build"
```

### 1.3 Gestor de paquetes — VCPKG

VCPKG resuelve automáticamente las dependencias de terceros (SFML y fmt) usando el toolchain de CMake. La ruta del toolchain se pasa como variable de configuración:

```json
"CMAKE_TOOLCHAIN_FILE": "c:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake",
"VCPKG_TARGET_TRIPLET": "x64-mingw-dynamic"
```

El triplet `x64-mingw-dynamic` indica que las bibliotecas se compilarán como DLLs de 64 bits para MinGW. Esto requiere que las DLLs de SFML y fmt estén disponibles en tiempo de ejecución (en la misma carpeta del ejecutable o en el PATH).

### 1.4 Biblioteca gráfica — SFML

SFML (_Simple and Fast Multimedia Library_) proporciona:

| Componente SFML  | Uso en el proyecto                                    |
| ---------------- | ----------------------------------------------------- |
| `SFML::Window`   | Gestión de la ventana y captura de eventos de teclado |
| `SFML::Graphics` | `sf::Texture`, `sf::Sprite`, `sf::Font`, `sf::Text`   |
| `SFML::System`   | Temporizador `sf::Clock` para medir FPS               |

La textura se actualiza cada frame a partir del buffer de píxeles `pixel_buffer` (arreglo `uint32_t` en formato RGBA):

```cpp
texture.update((const uint8_t *) pixel_buffer);
```

### 1.5 Biblioteca de formateo — fmt

La biblioteca **{fmt}** se usa para construir cadenas de texto de forma eficiente y segura, reemplazando `printf` o `std::stringstream`:

```cpp
auto msg = fmt::format("Julia Set: Iterations: {} | FPS: {} | Mode: {}", max_iteraciones, fps, mode);
```

### 1.6 Extensión de VS Code — CMake Tools

La extensión **CMake Tools** de Microsoft integra CMake directamente en VS Code, leyendo la configuración de `.vscode/settings.json`. Permite:

- Configurar el proyecto (equivalente a `cmake -B build`).
- Compilar por configuración desde la barra de estado.
- Lanzar y depurar el ejecutable directamente.

### 1.7 Instrucciones SIMD — AVX2

**AVX2** (_Advanced Vector Extensions 2_) es un conjunto de instrucciones disponible en procesadores Intel/AMD modernos que permite operar sobre **registros de 256 bits**. En este proyecto se usa para procesar **8 píxeles de forma simultánea** usando números de punto flotante de 32 bits (`float`), mediante el tipo `__m256` y los intrínsecos de `<immintrin.h>`.

---

## 2. Arquitectura del proyecto

### 2.1 Organización por tipo de archivo

```
001.fractal-Julia/
│
├── .vscode/
│   └── settings.json          ← Configuración de CMake Tools para VS Code
│
├── CMakeLists.txt             ← Script de build: target, dependencias, flags
│
├── main.cpp                   ← Punto de entrada, lazo principal SFML, eventos
├── fractal_serial.h           ← Interfaz: julia_serial_1, julia_serial_2
├── fractal_serial.cpp         ← Implementaciones seriales (double + std::complex)
├── fractal_simd.h             ← Interfaz: julia_simd
├── fractal_simd.cpp           ← Implementación AVX2 (float, 8 píxeles por iteración)
├── palette.h                  ← Constante PALETTE_SIZE (16), declaración color_ramp
└── palette.cpp                ← Definición de color_ramp y función bswap32
```

### 2.2 Descripción de cada archivo

#### `.vscode/settings.json`

Configura el entorno de CMake Tools para el espacio de trabajo:

- **`cmake.generator`**: `"Ninja Multi-Config"` — generador de archivos de build.
- **`cmake.buildDirectory`**: `"${workspaceFolder}/build"` — directorio de salida.
- **`cmake.configureSettings`**: variables de CMake inyectadas en la configuración:
  - `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` → GCC/G++ de MinGW-w64.
  - `CMAKE_MAKE_PROGRAM` → Ninja.
  - `CMAKE_TOOLCHAIN_FILE` → VCPKG.
  - `VCPKG_TARGET_TRIPLET` → `x64-mingw-dynamic`.

#### `CMakeLists.txt`

Define el proyecto y sus dependencias:

```cmake
cmake_minimum_required(VERSION 3.16)
project(FractalJulia)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")   # habilita AVX2

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(FractalJulia
    main.cpp fractal_serial.cpp palette.cpp fractal_simd.cpp)

target_link_libraries(FractalJulia PRIVATE
    fmt::fmt SFML::System SFML::Graphics SFML::Window)
```

Puntos relevantes:

- `-mavx2` se aplica globalmente al target.
- `fractal_simd.cpp` se incluye en el mismo target sin flags especiales adicionales (el flag global es suficiente).

#### `main.cpp`

Punto de entrada y lazo principal de la aplicación. Responsabilidades:

| Sección                  | Descripción                                                                       |
| ------------------------ | --------------------------------------------------------------------------------- |
| Parámetros globales      | `WIDTH=1920`, `HEIGHT=1080`, `max_iteraciones=10`, región compleja, constante `c` |
| `pixel_buffer`           | Arreglo `uint32_t[WIDTH*HEIGHT]` en heap; cada elemento es un color RGBA          |
| Enumerado `runtime_type` | `SERIAL_1`, `SERIAL_2`, `SIMD` — controla qué implementación ejecutar             |
| Lazo de eventos          | Teclas `Up`/`Down` (±10 iteraciones), `1`/`2`/`3` (selección de modo)             |
| Lazo de render           | Llama a la función seleccionada, actualiza textura, dibuja sprite y texto HUD     |
| Contador de FPS          | `sf::Clock` reiniciado cada segundo; muestra FPS en título de ventana             |
| HUD                      | Muestra iteraciones actuales, FPS y modo en pantalla                              |

Variables globales accedidas por los módulos de fractal:

```cpp
extern int max_iteraciones;        // límite de iteraciones
extern std::complex<double> c;     // constante del conjunto (-0.7 + 0.27015i)
```

#### `fractal_serial.h` / `fractal_serial.cpp`

Declara e implementa dos variantes seriales:

| Función pública       | Descripción                                                                 |
| --------------------- | --------------------------------------------------------------------------- |
| `julia_serial_1(...)` | Itera sobre todos los píxeles usando `std::complex<double>` y `acotado_1`   |
| `julia_serial_2(...)` | Itera sobre todos los píxeles con aritmética escalar `double` y `acotado_2` |

Funciones internas (no expuestas en el header):

| Función interna                      | Descripción                                                                    |
| ------------------------------------ | ------------------------------------------------------------------------------ |
| `acotado_1(std::complex<double> z0)` | Itera $z_{n+1} = z_n^2 + c$ usando operadores de `std::complex`; retorna color |
| `acotado_2(double x, double y)`      | Misma iteración con `double` separados (parte real e imaginaria explícitas)    |

Ambas funciones reciben la firma:

```cpp
void julia_serial_X(double x_min, double y_min, double x_max, double y_max,
                    uint32_t width, uint32_t height, uint32_t* pixel_buffer);
```

#### `fractal_simd.h` / `fractal_simd.cpp`

Declara e implementa la versión vectorizada:

| Función pública   | Descripción                                                               |
| ----------------- | ------------------------------------------------------------------------- |
| `julia_simd(...)` | Procesa 8 píxeles simultáneos (por columna `j`) usando registros `__m256` |

Estrategia interna:

- Para cada columna `i`, itera `j` en pasos de 8: `j += 8`.
- Carga 8 valores de coordenada imaginaria (`y`) en un registro `__m256`.
- La coordenada real (`x`) es la misma para los 8 píxeles del mismo `i`.
- Ejecuta la iteración de Julia en paralelo sobre los 8 píxeles.
- Usa máscaras de comparación (`_mm256_cmp_ps`) para detener selectivamente píxeles que ya escaparon.
- Detiene el lazo si todos los píxeles han escapado (`_mm256_testz_ps`).

Nota de precisión: usa `float` (32 bits) en lugar de `double` (64 bits). Los parámetros `double` de entrada se convierten implícitamente al crear los registros SIMD.

#### `palette.h` / `palette.cpp`

| Símbolo             | Tipo                    | Descripción                                                       |
| ------------------- | ----------------------- | ----------------------------------------------------------------- |
| `PALETTE_SIZE`      | `#define` (entero `16`) | Número de colores en la paleta                                    |
| `color_ramp`        | `std::vector<uint32_t>` | 16 colores RGBA; degradado de azul a rojo oscuro                  |
| `bswap32(uint32_t)` | función local           | Invierte el orden de bytes para convertir entre formatos de color |

El formato de almacenamiento en memoria es RGBA (esperado por `sf::Texture::update`). Los literales en el código están escritos como `0xRRGGBBAA`, por lo que `bswap32` los convierte al orden correcto: `0xAABBGGRR` → `0xRRGGBBAA`.

---

## 3. Chuleta de sintaxis C++

### 3.1 Variables, tipos y literales

```cpp
// Tipos enteros con tamaño explícito (cstdint)
uint32_t color = 0xFF0000FF;   // entero sin signo de 32 bits
int      iter  = 1;            // entero con signo

// Punto flotante
double x = -1.5;               // 64 bits — mayor precisión
float  f =  1.5f;              // 32 bits — sufijo 'f' obligatorio para float

// Constantes de preprocesador
#define WIDTH  1920
#define HEIGHT 1080
```

### 3.2 Punteros y memoria dinámica

```cpp
uint32_t* pixel_buffer = new uint32_t[WIDTH * HEIGHT]; // reservar en heap
pixel_buffer[j * WIDTH + i] = color;                   // acceso row-major
delete[] pixel_buffer;                                  // liberar arreglo
```

### 3.3 Números complejos — `<complex>`

```cpp
#include <complex>

std::complex<double> c(-0.7, 0.27015); // c = -0.7 + 0.27015i
std::complex<double> z(x, y);          // z = x + yi

z = z * z + c;            // iteración de Julia: z² + c
double norma = std::abs(z); // |z| — calcula raíz cuadrada
double re = c.real();       // parte real
double im = c.imag();       // parte imaginaria
```

### 3.4 Variables externas (`extern`)

```cpp
// En main.cpp — definición:
int max_iteraciones = 10;
std::complex<double> c(-0.7, 0.27015);

// En fractal_serial.cpp / fractal_simd.cpp — declaración:
extern int max_iteraciones;       // toma el valor de main.cpp
extern std::complex<double> c;
```

> `extern` declara que la variable existe en otra unidad de compilación. Sin `extern`, se crearía una variable nueva (error de enlace por símbolo duplicado o no encontrado).

### 3.5 Enumerados con ámbito (`enum`)

```cpp
enum runtime_type {
    SERIAL_1 = 0,
    SERIAL_2,
    SIMD
};

runtime_type r_type = runtime_type::SERIAL_1;

if (r_type == runtime_type::SIMD) { ... }
```

### 3.6 Macros de compilación condicional

```cpp
#ifdef _WIN32
    #include <windows.h>
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif
```

El bloque solo se compila en Windows. En Linux/macOS se ignora sin causar error.

### 3.7 Operadores a nivel de bits — paleta de colores

```cpp
// bswap32: invierte el orden de los 4 bytes de un uint32_t
uint32_t bswap32(uint32_t a) {
    return ((a & 0xFF000000) >> 24)   // byte 3 → byte 0
         | ((a & 0x00FF0000) >>  8)   // byte 2 → byte 1
         | ((a & 0x0000FF00) <<  8)   // byte 1 → byte 2
         | ((a & 0x000000FF) << 24);  // byte 0 → byte 3
}

// Ejemplo: 0xRRGGBBAA → 0xAABBGGRR (formato RGBA que espera SFML)
bswap32(0xFF1010FF); // azul opaco
```

| Operador | Nombre                   | Ejemplo        | Resultado |
| -------- | ------------------------ | -------------- | --------- |
| `&`      | AND bit a bit            | `0xFF & 0x0F`  | `0x0F`    |
| `\|`     | OR bit a bit             | `0xF0 \| 0x0F` | `0xFF`    |
| `>>`     | Desplazamiento derecha   | `0xFF00 >> 8`  | `0x00FF`  |
| `<<`     | Desplazamiento izquierda | `0x00FF << 8`  | `0xFF00`  |

### 3.8 Intrínsecos SIMD — `<immintrin.h>` (AVX2)

```cpp
#include <immintrin.h>

// Tipo: vector de 8 floats de 32 bits (256 bits en total)
__m256 reg;

// Cargar el mismo valor en las 8 posiciones
__m256 xmin = _mm256_set1_ps(-1.5f);

// Cargar 8 valores distintos (orden: posición 7,6,5,4,3,2,1,0)
__m256 my = _mm256_set_ps(j+7, j+6, j+5, j+4, j+3, j+2, j+1, j+0);

// Operaciones aritméticas elemento a elemento
__m256 suma  = _mm256_add_ps(a, b);  // a[i] + b[i]
__m256 resta = _mm256_sub_ps(a, b);  // a[i] - b[i]
__m256 prod  = _mm256_mul_ps(a, b);  // a[i] * b[i]

// Comparación → máscara (0xFFFFFFFF si verdadero, 0x00000000 si falso)
__m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ); // norma2 <= 4?

// AND lógico: aplicar máscara para actualizar solo elementos activos
mk = _mm256_add_ps(mk, _mm256_and_ps(mask, one)); // mk += mask ? 1 : 0

// Verificar si todos los elementos de la máscara son cero
if (_mm256_testz_ps(mask, _mm256_set1_ps(-1))) break; // todos escaparon

// Almacenar registro en arreglo de float
float d[8];
_mm256_storeu_ps(d, mk); // 'u' = sin requisito de alineación
```

### 3.9 `std::optional` y manejo de eventos SFML

```cpp
// pollEvent retorna std::optional<sf::Event>; vacío si no hay evento
while (const std::optional event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) window.close();

    if (event->is<sf::Event::KeyReleased>()) {
        // getIf retorna puntero al tipo concreto, o nullptr
        auto evt = event->getIf<sf::Event::KeyReleased>();
        switch (evt->scancode) {
            case sf::Keyboard::Scan::Up: max_iteraciones += 10; break;
            // ...
        }
    }
}
```

### 3.10 Formateo de cadenas con `fmt`

```cpp
#include <fmt/core.h>

// {} es placeholder posicional (equivalente a %d/%s pero seguro en tipos)
auto msg = fmt::format("Iter: {} | FPS: {} | Mode: {}", iter, fps, mode);
text.setString(msg); // pasar a sf::Text
```

---

## 4. Algoritmo del conjunto de Julia

### 4.1 Definición matemática

El conjunto de Julia asociado a una constante $c \in \mathbb{C}$ se define como el conjunto de puntos $z_0 \in \mathbb{C}$ para los cuales la órbita de la función:

$$z_{n+1} = z_n^2 + c$$

**no diverge** (es decir, permanece acotada). El criterio clásico de escape establece que si $|z_n| > 2$ para algún $n$, la órbita divergirá al infinito.

### 4.2 Parámetros del proyecto

| Parámetro         | Valor             | Descripción                                              |
| ----------------- | ----------------- | -------------------------------------------------------- |
| `c`               | $-0.7 + 0.27015i$ | Constante del conjunto de Julia                          |
| `x_min`, `x_max`  | $-1.5$, $1.5$     | Rango del eje real (parte real de $z_0$)                 |
| `y_min`, `y_max`  | $-1.0$, $1.0$     | Rango del eje imaginario (parte imaginaria de $z_0$)     |
| `WIDTH`, `HEIGHT` | $1920$, $1080$    | Resolución de la imagen en píxeles                       |
| `max_iteraciones` | 10 (inicial)      | Número máximo de iteraciones antes de considerar acotado |

### 4.3 Mapeo píxel → plano complejo

Cada píxel $(i, j)$ de la imagen se mapea al número complejo $z_0 = x + yi$ mediante:

$$x = x_{min} + i \cdot \frac{x_{max} - x_{min}}{W}, \quad y = y_{max} - j \cdot \frac{y_{max} - y_{min}}{H}$$

El eje $j$ (vertical) se invierte porque en pantalla $j=0$ es la parte superior, pero en coordenadas matemáticas $y$ crece hacia arriba.

### 4.4 Criterio de escape y coloración

```
Para cada z₀ = x + yi:
  z ← z₀
  iter ← 1
  Mientras iter < max_iteraciones Y |z|² < 4:
    z ← z² + c
    iter ← iter + 1

  Si iter < max_iteraciones:   → el punto escapó → color = color_ramp[iter % 16]
  Si iter == max_iteraciones:  → el punto NO escapó → color = negro (0xFF000000)
```

La coloración por número de iteración (método _escape time_) produce las bandas de color características del fractal.

### 4.5 Paleta de color

La paleta `color_ramp` contiene 16 entradas que van de azul (`0xFF1010FF`) a rojo oscuro (`0x1919A4FF` después de `bswap32`), generando un degradado que resalta las fronteras del conjunto. El índice de color se calcula como `iter % PALETTE_SIZE`, lo que crea repetición cíclica de la paleta cuando `max_iteraciones > 16`.

---

## 5. Comparación de implementaciones

### 5.1 Resumen comparativo

| Característica           | Serial 1               | Serial 2             | SIMD AVX2             |
| ------------------------ | ---------------------- | -------------------- | --------------------- |
| Función pública          | `julia_serial_1`       | `julia_serial_2`     | `julia_simd`          |
| Función interna          | `acotado_1`            | `acotado_2`          | — (interna al lazo)   |
| Tipo de dato             | `std::complex<double>` | `double` (escalares) | `__m256` (`float`)    |
| Precisión                | 64 bits (double)       | 64 bits (double)     | 32 bits (float)       |
| Píxeles por iteración    | 1                      | 1                    | 8                     |
| Vectorización explícita  | No                     | No                   | Sí (AVX2)             |
| Dependencias de cabecera | `<complex>`            | —                    | `<immintrin.h>`       |
| Portabilidad             | Alta                   | Alta                 | Requiere CPU con AVX2 |
| Legibilidad              | Alta                   | Media                | Baja                  |

### 5.2 Serial 1 — `julia_serial_1` con `std::complex`

```cpp
uint32_t acotado_1(std::complex<double> z0) {
    int iter = 1;
    std::complex<double> z = z0;
    while (iter < max_iteraciones && std::abs(z) < 2.0) {
        z = z*z + c;
        iter++;
    }
    if (iter < max_iteraciones) return color_ramp[iter % PALETTE_SIZE];
    return 0xFF000000;
}
```

**Ventajas:**

- Código directamente legible: la operación `z = z*z + c` es idéntica a la fórmula matemática.
- La librería estándar gestiona la aritmética de complejos.

**Desventajas:**

- `std::abs(z)` calcula una raíz cuadrada en cada iteración. Comparar `|z|² < 4` directamente (sin raíz) sería más eficiente.
- Los operadores de `std::complex` pueden generar código menos optimizado que la aritmética escalar directa.

### 5.3 Serial 2 — `julia_serial_2` con escalares

```cpp
uint32_t acotado_2(double x, double y) {
    int iter = 1;
    double zr = x, zi = y;
    while (iter < max_iteraciones && (zr*zr + zi*zi) < 4.0) {
        double dr = zr*zr - zi*zi + c.real();
        double di = 2.0*zr*zi + c.imag();
        zr = dr; zi = di;
        iter++;
    }
    if (iter < max_iteraciones) return color_ramp[iter % PALETTE_SIZE];
    return 0xFF000000;
}
```

**Ventajas:**

- Evita la raíz cuadrada: compara `zr²+zi² < 4` directamente.
- La aritmética escalar con `double` suele generar código más predecible para el compilador.
- Más cercano a lo que haría un compilador al vectorizar automáticamente.

**Desventajas:**

- Mayor verbosidad: la separación de partes real e imaginaria debe hacerse manualmente.

### 5.4 SIMD AVX2 — `julia_simd`

```cpp
// Procesa 8 píxeles (misma columna i, 8 filas consecutivas) en paralelo
for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j += 8) {
        __m256 cr = ...; // 8 veces el mismo x (misma columna)
        __m256 ci = ...; // 8 valores de y (filas j, j+1, ..., j+7)
        __m256 zr = cr, zi = ci;
        __m256 mk = _mm256_set1_ps(1.0f); // contadores de iteración

        while (iter < max_iteraciones) {
            // z² + c en paralelo para 8 puntos
            // actualizar mk solo para puntos que aún no escaparon
            // salir si todos escaparon
        }
        // asignar colores a los 8 píxeles
    }
}
```

**Ventajas:**

- Procesa **8 píxeles simultáneamente** usando registros de 256 bits con `float`.
- La máscara de escape permite salir anticipadamente si todos los puntos divergieron.
- Reduce significativamente el tiempo de render en CPUs modernas con AVX2.

**Desventajas:**

- Usa `float` (32 bits): menor precisión que `double`. Puede producir artefactos visuales a iteraciones muy altas o al hacer zoom.
- Los parámetros `x_min`, `x_max`, etc. son `double` en `main.cpp` pero se convierten a `float` al crear los registros `__m256`. Puede haber pérdida de precisión en los bordes del rango.
- Código más difícil de mantener: requiere conocimiento de intrínsecos AVX2.
- `height` debe ser múltiplo de 8. El código verifica `index < width*height` para manejar el caso contrario.
- No portátil: requiere un procesador con soporte AVX2 (Intel Haswell 2013+ / AMD Ryzen 2017+).

### 5.5 Análisis del lazo interno SIMD

El lazo SIMD utiliza las siguientes operaciones clave:

| Operación      | Intrínse                         | Descripción                                        |
| -------------- | -------------------------------- | -------------------------------------------------- |
| Multiplicación | `_mm256_mul_ps`                  | Producto elemento a elemento                       |
| Suma           | `_mm256_add_ps`                  | Suma elemento a elemento                           |
| Resta          | `_mm256_sub_ps`                  | Resta elemento a elemento                          |
| Comparación ≤  | `_mm256_cmp_ps(..., _CMP_LE_OQ)` | Máscara: 0xFFFFFFFF si verdadero                   |
| AND lógico     | `_mm256_and_ps`                  | Aplica máscara para actualizar solo puntos activos |
| Test cero      | `_mm256_testz_ps`                | Detecta si todos los puntos escaparon              |
| Almacenar      | `_mm256_storeu_ps`               | Vuelca registro a arreglo de float                 |

---

## 6. Observaciones técnicas y límites

### 6.1 Requisito de AVX2 en CPU

La función `julia_simd` usa `__m256` y `<immintrin.h>`. Si el ejecutable se lanza en una CPU sin AVX2, ocurrirá una instrucción ilegal en tiempo de ejecución. El flag `-mavx2` en CMake **no verifica** en tiempo de configuración si la CPU destino soporta AVX2; solo habilita la generación de ese código.

**Mitigación:** verificar soporte con `cpuid` en tiempo de ejecución, o documentar el requisito claramente.

### 6.2 Fuente arial.ttf

`main.cpp` carga `sf::Font font("arial.ttf")` usando una ruta relativa. Esto requiere que `arial.ttf` esté presente en el **directorio de trabajo** desde el que se lanza el ejecutable. Si se ejecuta directamente desde VS Code con CMake Tools, el directorio de trabajo es `build/<Config>/`. La fuente debe copiarse allí o el campo `cmake.debugConfig.cwd` debe ajustarse.

### 6.3 Precisión en SIMD vs serial

La SIMD usa `float` (precisión simple). Para la región $[-1.5, 1.5] \times [-1.0, 1.0]$ con las iteraciones típicas del proyecto, la diferencia visual es inapreciable. Sin embargo, al aumentar `max_iteraciones` o implementar zoom (reducción del rango complejo), los artefactos de precisión pueden volverse visibles.

### 6.4 Buffer de píxeles y orden de color

Los píxeles se almacenan en `uint32_t pixel_buffer[]`. La función `sf::Texture::update(const uint8_t*)` espera datos en orden **RGBA** (rojo en byte más bajo en memoria). La función `bswap32` en `palette.cpp` convierte los literales hexadecimales del formato `0xRRGGBBAA` al orden correcto de bytes para la arquitectura little-endian x86.

### 6.5 Tamaño del buffer y alineación

`pixel_buffer` es un arreglo plano de `WIDTH * HEIGHT = 2,073,600` elementos `uint32_t` (~8 MB). El acceso a píxel $(i, j)$ es `pixel_buffer[j * WIDTH + i]` (orden row-major, fila por fila). La SIMD llena por columna (`i` fijo, `j` variable en pasos de 8), lo que produce accesos **no consecutivos** en memoria para los 8 píxeles de un registro (stride = `WIDTH` enteros). Esto puede reducir ligeramente la eficiencia de caché en la escritura final.

### 6.6 Inicialización del buffer SIMD

```cpp
std::memset(pixel_buffer, 0xFF000000, width * height * sizeof(uint32_t));
```

> **Advertencia:** `std::memset` rellena **byte a byte** con el valor `0xFF000000 & 0xFF = 0x00`. Esto inicializa todos los bytes a `0x00`, dejando el buffer en negro transparente, no negro opaco. Para negro opaco (`0xFF000000`), se debería usar un lazo `std::fill` o `memset(buffer, 0, ...)`. El código funciona visualmente porque los píxeles no asignados se sobreescriben en el mismo frame.

---

## 7. Glosario

| Término                | Definición                                                                                                                                            |
| ---------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| **AVX2**               | _Advanced Vector Extensions 2_. Extensión del conjunto de instrucciones x86 que permite operar sobre 256 bits (8 floats o 4 doubles) simultáneamente. |
| **SIMD**               | _Single Instruction, Multiple Data_. Paradigma de paralelismo donde una sola instrucción opera sobre múltiples datos en paralelo.                     |
| **Intrínsecos**        | Funciones de C/C++ que mapean directamente a instrucciones del procesador (ej. `_mm256_add_ps`), sin necesidad de escribir ensamblador.               |
| **Escape time**        | Método de coloración de fractales basado en el número de iteraciones que tarda un punto en superar el criterio de escape.                             |
| **Conjunto de Julia**  | Conjunto de puntos del plano complejo cuya órbita bajo $f(z) = z^2 + c$ no diverge. Varía con $c$.                                                    |
| **Ninja Multi-Config** | Generador de CMake que produce un único directorio de build con múltiples configuraciones (Debug, Release, etc.).                                     |
| **VCPKG**              | Gestor de paquetes C++ de Microsoft; resuelve y compila dependencias de terceros integrado con CMake.                                                 |
| **Triplet VCPKG**      | Especificación de plataforma, arquitectura y tipo de enlace para compilar dependencias (ej. `x64-mingw-dynamic`).                                     |
| **RGBA**               | Formato de color con canales Rojo, Verde, Azul y Alfa (transparencia), almacenados en 4 bytes por píxel.                                              |
| **`bswap32`**          | Inversión del orden de bytes de un entero de 32 bits; se usa para convertir entre formatos de color en distintas convenciones de endianness.          |
