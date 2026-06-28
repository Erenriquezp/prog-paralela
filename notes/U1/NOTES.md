# Notas de Estudio — Programacion Paralela

## Mapa del repositorio

| Modulo                    | Lenguaje | Que contiene                                                     |
| ------------------------- | -------- | ---------------------------------------------------------------- |
| `ejemplo01` / `ejemplo02` | C++      | Base SFML: ventana, eventos, texto. Pruebas de plataforma.       |
| `001.fractal-Julia`       | C++      | App completa: Serial 1, Serial 2, SIMD AVX2 con SFML.            |
| `03.fractal-dll`          | C++      | Kernel SIMD exportado como DLL (`extern "C" __stdcall`).         |
| `02.fractal-java`         | Java     | App completa: CPU serial, Threads, llamada a la DLL via JNR-FFI. |

Flujo de un frame en ambas apps:

```
Input teclado → seleccionar modo → calcular fractal → subir textura → mostrar frame
```

---

## Matematica: Conjunto de Julia

### Definicion

El conjunto de Julia de una constante $c \in \mathbb{C}$ es el conjunto de puntos $z_0 \in \mathbb{C}$ para los que la orbita de la funcion

$$z_{n+1} = z_n^2 + c$$

**no diverge**. En este proyecto se usa $c = -0.7 + 0.27015i$.

### Expansion escalar de $z^2$

Dado $z = z_r + z_i \cdot i$:

$$z^2 = (z_r^2 - z_i^2) + (2 z_r z_i) \cdot i$$

Por tanto la iteracion descompuesta es:

$$z_r' = z_r^2 - z_i^2 + c_r \qquad z_i' = 2 z_r z_i + c_i$$

Esta forma escalar es la que usan `Serial 2` y `SIMD` (y evita el overhead de `std::complex`).

### Criterio de escape

Teorema: si $|z_n| > 2$ para algun $n$, la orbita diverge al infinito.

En codigo se evita la raiz cuadrada comparando el cuadrado de la norma:

$$|z|^2 = z_r^2 + z_i^2 < 4$$

```
iter ← 1
Mientras iter < max_iteraciones  Y  zr² + zi² < 4:
    zr', zi' ← iteracion escalar
    iter ← iter + 1

Si iter < max_iteraciones  →  escapo  →  color_ramp[iter % 16]
Si iter == max_iteraciones →  acotado  →  negro (0xFF000000)
```

### Mapeo pixel → plano complejo

Cada pixel $(i, j)$ de la imagen (0-indexed) se convierte en $z_0 = x + y \cdot i$:

$$dx = \frac{x_{max} - x_{min}}{W} \qquad dy = \frac{y_{max} - y_{min}}{H}$$

$$x = x_{min} + i \cdot dx \qquad y = y_{max} - j \cdot dy$$

El eje $j$ se invierte porque en pantalla $j=0$ es la parte superior, pero matematicamente $y$ crece hacia arriba.

**Parametros del proyecto:**

| Parametro         | C++                    | Java                   |
| ----------------- | ---------------------- | ---------------------- |
| Resolucion        | 1920 × 1080            | 1600 × 900             |
| Rango real        | $[-1.5,\ 1.5]$         | $[-1.5,\ 1.5]$         |
| Rango imag        | $[-1.0,\ 1.0]$         | $[-1.0,\ 1.0]$         |
| $c$               | $-0.7 + 0.27015i$      | $-0.7 + 0.27015i$      |
| `max_iteraciones` | 10 (inicial, variable) | 10 (inicial, variable) |

---

## Implementaciones C++

### Comparacion rapida

|              | Serial 1               | Serial 2         | SIMD AVX2          |
| ------------ | ---------------------- | ---------------- | ------------------ |
| Tipo de dato | `std::complex<double>` | `double` escalar | `__m256` (`float`) |
| Precision    | 64 bits                | 64 bits          | 32 bits            |
| Pixeles/iter | 1                      | 1                | 8                  |
| Legibilidad  | Alta                   | Media            | Baja               |
| Portabilidad | Alta                   | Alta             | Requiere AVX2      |

---

### Serial 1 — `std::complex<double>`

La iteracion es literalmente la formula matematica.

```cpp
// fractal_serial.cpp
uint32_t acotado_1(std::complex<double> z0) {
    int iter = 1;
    std::complex<double> z = z0;
    while (iter < max_iteraciones && std::abs(z) < 2.0) {
        z = z * z + c;
        iter++;
    }
    if (iter < max_iteraciones) return color_ramp[iter % PALETTE_SIZE];
    return 0xFF000000;
}
```

**Costo oculto:** `std::abs(z)` calcula una raiz cuadrada en cada iteracion. Serial 2 lo evita.

---

### Serial 2 — escalares `double`

Misma logica, pero descompone el complejo manualmente. Evita la raiz cuadrada y reduce abstraccion.

```cpp
uint32_t acotado_2(double x, double y) {
    int iter = 1;
    double zr = x, zi = y;
    while (iter < max_iteraciones && (zr*zr + zi*zi) < 4.0) {
        double dr = zr*zr - zi*zi + c.real();
        double di = 2.0*zr*zi + c.imag();
        zr = dr;
        zi = di;
        iter++;
    }
    if (iter < max_iteraciones) return color_ramp[iter % PALETTE_SIZE];
    return 0xFF000000;
}
```

**Diferencia clave vs Serial 1:** compara `zr²+zi² < 4` (sin raiz). El compilador puede optimizar mejor aritmetica escalar directa que operadores de `std::complex`.

---

### SIMD AVX2 — 8 pixeles simultaneos

`__m256` es un registro de 256 bits que almacena 8 `float` en paralelo. La idea: procesar una columna `i` fija con 8 filas consecutivas `j..j+7` a la vez.

```cpp
// fractal_simd.cpp — lazo externo
for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j += 8) {       // 8 pixeles por iteracion

        // Coordenadas: x igual para los 8, y distinta para cada fila
        __m256 cr = _mm256_add_ps(xmin, _mm256_mul_ps(_mm256_set1_ps(i), xscale));
        __m256 ci = _mm256_sub_ps(ymax, _mm256_mul_ps(
                        _mm256_set_ps(j+7,j+6,j+5,j+4,j+3,j+2,j+1,j+0), yscale));

        __m256 zr = cr, zi = ci;
        __m256 mk = _mm256_set1_ps(1.0f);        // contadores de iteracion

        while (iter < max_iteraciones) {
            __m256 zr2  = _mm256_mul_ps(zr, zr);
            __m256 zi2  = _mm256_mul_ps(zi, zi);
            __m256 zrzi = _mm256_mul_ps(zr, zi);

            zr = _mm256_add_ps(_mm256_sub_ps(zr2, zi2), c_real);  // zr² - zi² + cr
            zi = _mm256_add_ps(_mm256_add_ps(zrzi, zrzi), c_imag); // 2*zr*zi + ci

            __m256 norma2 = _mm256_add_ps(_mm256_mul_ps(zr,zr), _mm256_mul_ps(zi,zi));

            // mascara: 0xFFFFFFFF para lanes que aun no escaparon
            __m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ);

            // incrementar solo los lanes activos
            mk = _mm256_add_ps(mk, _mm256_and_ps(mask, one));

            // salida temprana si todos escaparon
            if (_mm256_testz_ps(mask, _mm256_set1_ps(-1))) break;
            iter++;
        }

        // volcar contadores y asignar colores a los 8 pixeles
        float d[8];
        _mm256_storeu_ps(d, mk);
        for (int it = 0; it < 8; it++) { /* asignar color_ramp o negro */ }
    }
}
```

**Intrinsecos clave:**

| Intrinseco                      | Operacion                                                    |
| ------------------------------- | ------------------------------------------------------------ |
| `_mm256_set1_ps(v)`             | Llenar los 8 lanes con el mismo valor `v`                    |
| `_mm256_set_ps(v7..v0)`         | Cargar 8 valores distintos (orden: lane 7 primero)           |
| `_mm256_mul_ps(a,b)`            | `a[i] * b[i]` para i en 0..7                                 |
| `_mm256_add_ps(a,b)`            | `a[i] + b[i]` para i en 0..7                                 |
| `_mm256_sub_ps(a,b)`            | `a[i] - b[i]` para i en 0..7                                 |
| `_mm256_cmp_ps(a,b,_CMP_LE_OQ)` | Mascara: `0xFFFFFFFF` si `a[i] <= b[i]`, `0x0` si no         |
| `_mm256_and_ps(a,mask)`         | `a[i]` si mascara activa, `0.0` si no                        |
| `_mm256_testz_ps(mask,mask)`    | Retorna 1 (true) si todos los bits de la mascara son 0       |
| `_mm256_storeu_ps(ptr,reg)`     | Vuelca los 8 floats del registro a un array (sin alineacion) |

**Por que `float` y no `double`:** AVX2 con `float` procesa 8 valores por registro. Con `double` solo procesaria 4 (`__m256d`). Se sacrifica precision a cambio del doble de throughput.

---

## Implementaciones Java

### CPU serial — `julia_serial_2`

Identico a `Serial 2` de C++. Es el baseline para comparar dentro de la JVM.

```java
// FractalCpu.java
int acotado_2(double x, double y) {
    int iter = 1;
    double zr = x, zi = y;
    while (iter < FractalParams.maxIteraciones && (zr*zr + zi*zi) < 4.0) {
        double dr = zr*zr - zi*zi + FractalParams.cReal;
        double di = 2.0*zr*zi + FractalParams.cImag;
        zr = dr; zi = di;
        iter++;
    }
    if (iter < FractalParams.maxIteraciones) return FractalParams.colorRamp[iter % PALETTE_SIZE];
    return 0xFF000000;
}
```

---

### Multihilo por filas — `julia_threads_2`

Divide las filas de la imagen entre tantos hilos como nucleos disponibles. Cada hilo escribe en su rango exclusivo de `pixelBuffer`, lo que elimina condiciones de carrera sin necesitar `synchronized`.

```java
int cores = Runtime.getRuntime().availableProcessors();
int threadCount = Math.min(cores, height);
int rowsPerThread = height / threadCount;
int remainder = height % threadCount;
int startRow = 0;

for (int t = 0; t < threadCount; t++) {
    int extra = (t < remainder) ? 1 : 0;   // distribuir filas sobrantes
    int endRow = startRow + rowsPerThread + extra;
    final int rowStart = startRow;
    final int rowEnd = endRow;

    threads[t] = new Thread(() -> {
        for (int j = rowStart; j < rowEnd; j++) {
            for (int i = 0; i < width; i++) {
                double x = x_min + i * dx;
                double y = y_max - j * dy;
                pixelBuffer[j * width + i] = acotado_threads_2(x, y);
            }
        }
    });
    threads[t].start();
    startRow = endRow;
}
for (Thread th : threads) th.join();   // esperar a todos antes de continuar
```

**Por que no hay race condition:** cada hilo escribe indices `[rowStart*width .. rowEnd*width-1]` que no se solapan con los de los demas hilos.

---

### DLL SIMD via JNR-FFI — `juliaSimd`

Tres piezas que trabajan juntas:

**1. Interfaz Java (`FractalDll.java`) — define la firma de la funcion nativa:**

```java
public interface FractalDll {
    String LIBRARY_NAME = "libfractal-dll";
    FractalDll INSTANCE = LibraryLoader.create(FractalDll.class).load(LIBRARY_NAME);

    void julia_simd(double x_min, double y_min, double x_max, double y_max,
                    int width, int height, int max_iteraciones, Buffer pixel_buffer);
}
```

**2. Buffer de memoria compartida (`FractalSimd.java`) — `ByteBuffer` directo:**

```java
// allocateDirect: memoria fuera del heap de la JVM (accesible desde C++ sin copias)
// nativeOrder: asegura little-endian en Windows, coincide con lo que escribe la DLL
this.pixelBuffer = ByteBuffer.allocateDirect(WIDTH * HEIGHT * Integer.BYTES)
                             .order(ByteOrder.nativeOrder());
```

**3. DLL C++ (`03.fractal-dll/fractal_simd.cpp`) — funcion exportada:**

```cpp
extern "C" __stdcall   // sin name mangling + convencion de llamada Windows
void julia_simd(double x_min, double y_min, double x_max, double y_max,
                uint32_t width, uint32_t height,
                int max_iteraciones, uint32_t* pixel_buffer) { /* ... */ }
```

- `extern "C"`: evita el name mangling de C++ para que JNR-FFI pueda encontrar el simbolo por nombre.
- `__stdcall`: convencion de llamada estandar de Windows; el callee limpia el stack.

**Verificar simbolo exportado:**

```powershell
objdump -p Release/libfractal-dll.dll | grep julia_simd
```

---

## Paleta de color y formato de pixel

`color_ramp` tiene 16 colores RGBA. El indice se calcula como `iter % 16`, creando repeticion ciclica de la paleta cuando `max_iteraciones > 16`.

**Formato de pixel:** `sf::Texture::update` espera bytes en orden `RGBA` (rojo en byte mas bajo en memoria). Los literales del codigo estan escritos como `0xRRGGBBAA`, por lo que `bswap32` los invierte antes de almacenarlos:

```cpp
uint32_t bswap32(uint32_t a) {
    return ((a & 0xFF000000) >> 24)   // byte 3 → byte 0
         | ((a & 0x00FF0000) >>  8)   // byte 2 → byte 1
         | ((a & 0x0000FF00) <<  8)   // byte 1 → byte 2
         | ((a & 0x000000FF) << 24);  // byte 0 → byte 3
}
```

**Buffer plano row-major:** acceso a pixel $(i, j)$ → `pixel_buffer[j * WIDTH + i]`.

---

## Entorno de build

### C++: CMake + Ninja Multi-Config + VCPKG

```cmake
# CMakeLists.txt (clave)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")   # habilita AVX2 globalmente

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(FractalJulia main.cpp fractal_serial.cpp palette.cpp fractal_simd.cpp)
target_link_libraries(FractalJulia PRIVATE fmt::fmt SFML::System SFML::Graphics SFML::Window)
```

| Variable de configuracion | Valor                          |
| ------------------------- | ------------------------------ |
| `cmake.generator`         | `Ninja Multi-Config`           |
| `VCPKG_TARGET_TRIPLET`    | `x64-mingw-dynamic`            |
| `CMAKE_CXX_COMPILER`      | `c:/tools/mingw64/bin/g++.exe` |

Ninja Multi-Config genera una sola carpeta `build/` con subcarpetas `Debug/`, `Release/`, `RelWithDebInfo/` simultaneamente.

### Java: Gradle Kotlin DSL

```kotlin
// build.gradle.kts (dependencias clave)
implementation("org.lwjgl", "lwjgl-opengl")      // contexto OpenGL
implementation("org.lwjgl", "lwjgl-glfw")         // ventana y eventos
runtimeOnly("org.lwjgl:lwjgl::natives-windows")  // binarios nativos Windows
implementation("com.github.jnr:jnr-ffi:2.2.19")  // FFI para llamar la DLL
```

Main class: `com.programacion.paralela.FractalMain`

---

## Sintaxis critica

### `extern` — variables compartidas entre archivos

```cpp
// main.cpp — DEFINICION (reserva memoria)
int max_iteraciones = 10;
std::complex<double> c(-0.7, 0.27015);

// fractal_serial.cpp / fractal_simd.cpp — DECLARACION (usa la misma variable)
extern int max_iteraciones;
extern std::complex<double> c;
```

Sin `extern` en los otros archivos, el enlazador encontraria simbolos duplicados o faltantes.

### `std::optional` y eventos SFML 3

```cpp
while (const std::optional event = window.pollEvent()) {  // vacio si no hay evento
    if (event->is<sf::Event::Closed>()) window.close();
    if (event->is<sf::Event::KeyReleased>()) {
        auto evt = event->getIf<sf::Event::KeyReleased>(); // puntero al tipo concreto
        switch (evt->scancode) {
            case sf::Keyboard::Scan::Up:   max_iteraciones += 10; break;
            case sf::Keyboard::Scan::Num1: r_type = SERIAL_1;     break;
        }
    }
}
```

### `enum` para seleccionar modo de ejecucion

```cpp
enum runtime_type { SERIAL_1 = 0, SERIAL_2, SIMD };
runtime_type r_type = SERIAL_1;

if      (r_type == SERIAL_1) julia_serial_1(...);
else if (r_type == SERIAL_2) julia_serial_2(...);
else if (r_type == SIMD)     julia_simd(...);
```

### Compilacion condicional por plataforma

```cpp
#ifdef _WIN32
    #include <windows.h>
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif
```

Solo se compila en Windows; en Linux/macOS se ignora sin error.

---

## Advertencias y trampas

| Problema                             | Causa                                                                                      | Solucion                                                                                |
| ------------------------------------ | ------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------- |
| `memset` no inicializa a negro opaco | `memset` rellena byte a byte: `0xFF000000 & 0xFF = 0x00` → negro transparente              | Usar `std::fill(buf, buf+n, 0xFF000000u)`                                               |
| `arial.ttf` no encontrado            | `font("arial.ttf")` usa ruta relativa al directorio de trabajo                             | Copiar `arial.ttf` a `build/Release/` o configurar `cmake.debugConfig.cwd`              |
| Artefactos visuales con SIMD         | `float` (32 bits) vs `double` (64 bits): diferencia visible a iteraciones altas o con zoom | Aceptable para el rango actual; usar `__m256d` si se necesita mayor precision           |
| Crash en CPU sin AVX2                | `-mavx2` genera instrucciones ilegales si la CPU no las soporta                            | Intel Haswell (2013+) y AMD Ryzen (2017+) soportan AVX2                                 |
| `height` no multiplo de 8            | El lazo SIMD itera `j += 8`; puede salirse del buffer                                      | El codigo verifica `index < width*height` antes de escribir                             |
| Colores incorrectos en FFI           | Diferencia de endianness entre C++ y Java                                                  | `ByteOrder.nativeOrder()` en el `ByteBuffer` de Java garantiza little-endian en Windows |

---

## Comandos de build

```powershell
# C++ — app completa
cmake -S 001.fractal-Julia -B 001.fractal-Julia/build -G "Ninja Multi-Config"
cmake --build 001.fractal-Julia/build --config Release

# C++ — DLL para Java
cmake -S 03.fractal-dll -B 03.fractal-dll/build -G "Ninja Multi-Config"
cmake --build 03.fractal-dll/build --config Release

# Java
cd 02.fractal-java
.\gradlew clean build
.\gradlew run
```
