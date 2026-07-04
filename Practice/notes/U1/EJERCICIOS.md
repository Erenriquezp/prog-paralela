# EJERCICIOS — Guia Tecnica del Codigo

Cada ejercicio muestra el codigo real del proyecto con anotaciones inline, seguido de los puntos clave a retener. Los ejercicios estan ordenados en capas: de lo mas simple a lo mas complejo.

---

## Bloque A — Entorno y base SFML

### EJ-01 | `ejemplo01/main.cpp` — Plataforma: ventana + OpenMP

> **Observar:** como SFML 3 crea ventana con `VideoMode`, como el loop de eventos usa `std::optional`, y como verificar el entorno de paralelismo con OpenMP.

```cpp
#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <optional>          // requerido para el sistema de eventos de SFML 3

int main() {
    // Consulta al SO cuantos hilos logicos hay disponibles
    int max_threads = omp_get_max_threads();
    fmt::print("Sistema listo. Hilos disponibles: {}\n", max_threads);

    // SFML 3: el tamanio de ventana es un Vector2u {ancho, alto}
    sf::RenderWindow window(sf::VideoMode({400, 200}), "UCE - Prog. Paralela");

    while (window.isOpen()) {
        // pollEvent retorna std::optional<sf::Event>: vacio si no hay evento
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        window.clear(sf::Color(30, 30, 30));
        window.display();
    }
    return 0;
}
```

**Puntos clave:**

- `omp_get_max_threads()` no crea hilos; solo consulta cuantos estan disponibles.
- `std::optional` de `pollEvent` devuelve vacio cuando no hay eventos pendientes — el bucle `while` lo explota como condicion de corte.
- `window.clear()` + `window.display()` es el ciclo minimo de render: limpiar buffer trasero → presentar.

---

### EJ-02 | `ejemplo02/main.cpp` — Render: fuente + texto

> **Observar:** como se agregan recursos graficos (fuente y texto) al pipeline de render sin cambiar la estructura del loop.

```cpp
sf::RenderWindow window(sf::VideoMode({800, 600}), "UCE - Prog. Paralela");

// Font y Text deben existir mientras la ventana este abierta
const sf::Font font("arial.ttf");          // carga desde directorio de trabajo
sf::Text text(font, "Hello SFML", 50);    // asocia fuente, string y tamanio

while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) window.close();
    }
    window.clear(sf::Color(30, 30, 30));
    window.draw(text);    // dibuja texto sobre el frame
    window.display();
}
```

**Puntos clave:**

- El orden `clear → draw → display` es fijo; cualquier `draw` entre `clear` y `display` aparece en pantalla.
- `sf::Font` y `sf::Text` son recursos que deben estar en scope durante todo el loop — si se destruyen antes, el render falla.
- La ruta `"arial.ttf"` es relativa al **directorio de trabajo** del ejecutable, no al fuente.

---

## Bloque B — App C++: orquestacion (`001.fractal-Julia`)

### EJ-03 | `main.cpp` — Parametros globales y buffer de pixeles

> **Observar:** como se declaran los parametros que gobiernan el fractal y como se asigna el buffer que recibiran todos los kernels.

```cpp
// Resolucion: igual a la textura que se sube a SFML
#define WIDTH  1920
#define HEIGHT 1080

// Parametros del fractal — modificables en tiempo real con Up/Down
int max_iteraciones = 10;

// Region del plano complejo a renderizar
double x_min = -1.5,  x_max = 1.5;
double y_min = -1.0,  y_max = 1.0;

// Constante del conjunto de Julia
std::complex<double> c(-0.7, 0.27015);

// Buffer plano [WIDTH * HEIGHT]: cada uint32_t es un pixel en formato RGBA
uint32_t* pixel_buffer = new uint32_t[WIDTH * HEIGHT];
```

```cpp
// En fractal_serial.cpp y fractal_simd.cpp se accede a estas variables con:
extern int max_iteraciones;
extern std::complex<double> c;
// Sin 'extern' el enlazador generaria error de simbolo duplicado o faltante.
```

**Puntos clave:**

- `pixel_buffer` es un arreglo plano row-major: pixel $(i,j)$ → `pixel_buffer[j * WIDTH + i]`.
- `extern` en los archivos de kernel no reserva memoria; solo declara que la variable existe en otra unidad de compilacion (`main.cpp`).
- Cambiar `max_iteraciones`, `x_min/x_max`, `y_min/y_max` o `c` en tiempo real produce un resultado visual diferente en el siguiente frame.

---

### EJ-04 | `main.cpp` — Selector de modo y eventos de teclado

> **Observar:** como un `enum` controla que kernel se ejecuta, y como los eventos de teclado modifican el estado global de forma inmediata.

```cpp
enum runtime_type { SERIAL_1 = 0, SERIAL_2, SIMD };
runtime_type r_type = runtime_type::SERIAL_1;
```

```cpp
// Dentro del loop de eventos:
else if (event->is<sf::Event::KeyReleased>()) {
    auto evt = event->getIf<sf::Event::KeyReleased>();  // puntero al tipo concreto

    switch (evt->scancode) {
        case sf::Keyboard::Scan::Up:   max_iteraciones += 10;            break;
        case sf::Keyboard::Scan::Down:
            max_iteraciones -= 10;
            if (max_iteraciones < 10) max_iteraciones = 10;              break;
        case sf::Keyboard::Scan::Num1: r_type = runtime_type::SERIAL_1; break;
        case sf::Keyboard::Scan::Num2: r_type = runtime_type::SERIAL_2; break;
        case sf::Keyboard::Scan::Num3: r_type = runtime_type::SIMD;     break;
        default: break;
    }
}
```

| Tecla  | Efecto                               |
| ------ | ------------------------------------ |
| `Up`   | `max_iteraciones += 10`              |
| `Down` | `max_iteraciones -= 10` (minimo 10)  |
| `1`    | Activa Serial 1 (`std::complex`)     |
| `2`    | Activa Serial 2 (escalares `double`) |
| `3`    | Activa SIMD AVX2                     |

**Puntos clave:**

- `KeyReleased` (no `KeyPressed`) evita repeticion automatica del SO al mantener la tecla.
- `getIf<T>()` retorna un puntero al tipo de evento concreto; si el evento no es ese tipo, retorna `nullptr`.
- El `enum` actua como selector de estrategia: `main.cpp` orquesta, los archivos de kernel no se conocen entre si.

---

### EJ-05 | `main.cpp` — Pipeline de render por frame

> **Observar:** el ciclo completo de un frame: calcular fractal → subir textura → contar FPS → dibujar.

```cpp
// --- Calcular fractal (segun modo seleccionado) ---
std::string mode;
if (r_type == runtime_type::SERIAL_1) {
    julia_serial_1(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
    mode = "Serial 1";
} else if (r_type == runtime_type::SERIAL_2) {
    julia_serial_2(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
    mode = "Serial 2";
} else if (r_type == runtime_type::SIMD) {
    julia_simd(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
    mode = "SIMD";
}

// --- Subir pixel_buffer a la textura de GPU ---
texture.update((const uint8_t*) pixel_buffer);  // cast: uint32_t* → uint8_t* (mismo bloque de memoria)

// --- Contar FPS: cada segundo, fps = cantidad de frames del segundo anterior ---
frames++;
if (clock.getElapsedTime().asSeconds() >= 1.0f) {
    fps = frames;
    frames = 0;
    clock.restart();
}

// --- Construir HUD y dibujar ---
auto msg = fmt::format("Julia Set: Iterations: {} | FPS: {} | Mode: {}", max_iteraciones, fps, mode);
text.setString(msg);

window.clear();
window.draw(sprite);       // el fractal (sprite usa la textura actualizada)
window.draw(text);         // HUD superior
window.draw(textOptions);  // HUD inferior con controles
window.display();
```

**Puntos clave:**

- `texture.update()` copia el buffer de CPU a memoria de GPU — es parte del costo real por frame, no solo el kernel matematico.
- El cast `(const uint8_t*)` funciona porque `texture.update` espera bytes crudos; el layout del `uint32_t` en memoria little-endian coincide con RGBA.
- `sf::Clock` mide tiempo de pared; `clock.restart()` devuelve el tiempo transcurrido y reinicia el contador.

---

## Bloque C — Kernels de fractal C++

### EJ-06 | `fractal_serial.cpp` — Serial 1: iteracion con `std::complex`

> **Observar:** la traduccion directa de la formula matematica $z_{n+1} = z_n^2 + c$ a codigo.

```cpp
// Funcion auxiliar: recibe z0 y retorna el color del pixel
uint32_t acotado_1(std::complex<double> z0) {
    int iter = 1;
    std::complex<double> z = z0;

    // std::abs(z) calcula sqrt(zr^2 + zi^2) — hay una raiz cuadrada por iteracion
    while (iter < max_iteraciones && std::abs(z) < 2.0) {
        z = z * z + c;   // literalmente la formula del conjunto de Julia
        iter++;
    }

    if (iter < max_iteraciones)
        return color_ramp[iter % PALETTE_SIZE];  // escapo: color por iteracion
    return 0xFF000000;                           // no escapo: negro opaco
}

// Funcion publica: recorre todos los pixeles y llama a acotado_1
void julia_serial_1(double x_min, double y_min, double x_max, double y_max,
                    uint32_t width, uint32_t height, uint32_t* pixel_buffer) {
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            double x = x_min + i * dx;
            double y = y_max - j * dy;      // j invertido: origen en esquina superior izquierda

            std::complex<double> z(x, y);
            pixel_buffer[j * width + i] = acotado_1(z);
        }
    }
}
```

**Puntos clave:**

- `std::abs(z)` implica una raiz cuadrada en cada iteracion — costo evitable comparando `|z|^2 < 4` (Serial 2 lo hace).
- La inversion de `j` (`y = y_max - j * dy`) alinea el eje matematico (crece hacia arriba) con el eje de pantalla (crece hacia abajo).
- `pixel_buffer[j * width + i]` es el acceso row-major: fila `j`, columna `i`.

---

### EJ-07 | `fractal_serial.cpp` — Serial 2: iteracion escalar `double`

> **Observar:** la misma logica que Serial 1 pero con la expansion manual de $z^2$, evitando la raiz cuadrada y la abstraccion de `std::complex`.

```cpp
uint32_t acotado_2(double x, double y) {
    int iter = 1;
    double zr = x, zi = y;

    // zr^2 + zi^2 < 4  <==>  |z| < 2  (sin raiz cuadrada)
    while (iter < max_iteraciones && (zr*zr + zi*zi) < 4.0) {
        // Expansion de (zr + zi*i)^2 + c:
        double dr = zr*zr - zi*zi + c.real();   // parte real
        double di = 2.0*zr*zi  + c.imag();      // parte imaginaria
        zr = dr;
        zi = di;
        iter++;
    }

    if (iter < max_iteraciones)
        return color_ramp[iter % PALETTE_SIZE];
    return 0xFF000000;
}
```

**Comparacion directa con Serial 1:**

| Aspecto                | Serial 1               | Serial 2                    |
| ---------------------- | ---------------------- | --------------------------- |
| Condicion de escape    | `std::abs(z) < 2.0`    | `zr*zr + zi*zi < 4.0`       |
| Iteracion              | `z = z*z + c`          | expansion manual `dr`, `di` |
| Raiz cuadrada por iter | Si                     | No                          |
| Tipo de argumento      | `std::complex<double>` | `double x, double y`        |

**Puntos clave:**

- `zr*zr + zi*zi < 4` es matematicamente identico a `|z| < 2` pero sin calcular raiz cuadrada.
- El compilador puede vectorizar automaticamente la aritmetica escalar de `double` mejor que los operadores de `std::complex`.

---

### EJ-08 | `fractal_simd.cpp` — SIMD AVX2: 8 pixeles simultaneos

> **Observar:** como se vectoriza la iteracion de Julia para procesar 8 pixeles en paralelo con registros `__m256`, y como la mascara de escape implementa salida selectiva por lane.

#### Setup de registros constantes (una vez por funcion)

```cpp
// Todos los registros constantes se inicializan fuera del lazo principal
__m256 xmin      = _mm256_set1_ps(x_min);    // {x_min, x_min, ..., x_min}  (8 veces)
__m256 ymax      = _mm256_set1_ps(y_max);
__m256 xscale    = _mm256_set1_ps(dx);
__m256 yscale    = _mm256_set1_ps(dy);
__m256 c_real    = _mm256_set1_ps(c.real());
__m256 c_imag    = _mm256_set1_ps(c.imag());
__m256 max_norma = _mm256_set1_ps(4.0f);     // umbral de escape al cuadrado
__m256 one       = _mm256_set1_ps(1.0f);     // para incrementar contadores
```

#### Lazo principal: columna `i`, bloque de 8 filas `j..j+7`

```cpp
for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j += 8) {   // paso de 8: un bloque por iteracion

        // La coordenada x es la misma para los 8 pixeles (misma columna)
        __m256 mx = _mm256_set1_ps(i);
        // La coordenada y es distinta para cada pixel (8 filas consecutivas)
        __m256 my = _mm256_set_ps(j+7, j+6, j+5, j+4, j+3, j+2, j+1, j+0);

        __m256 cr = _mm256_add_ps(xmin, _mm256_mul_ps(mx, xscale)); // x = xmin + i*dx
        __m256 ci = _mm256_sub_ps(ymax, _mm256_mul_ps(my, yscale)); // y = ymax - j*dy

        int iter = 1;
        __m256 mk = _mm256_set1_ps(1.0f);   // contadores de iteracion: uno por lane
        __m256 zr = cr, zi = ci;            // z0 = punto inicial de cada pixel

        while (iter < max_iteraciones) {
            __m256 zr2  = _mm256_mul_ps(zr, zr);           // zr^2
            __m256 zi2  = _mm256_mul_ps(zi, zi);           // zi^2
            __m256 zrzi = _mm256_mul_ps(zr, zi);           // zr*zi

            zr = _mm256_add_ps(_mm256_sub_ps(zr2, zi2), c_real);      // zr^2 - zi^2 + cr
            zi = _mm256_add_ps(_mm256_add_ps(zrzi, zrzi), c_imag);    // 2*zr*zi + ci

            // Norma al cuadrado de z actualizado
            __m256 norma2 = _mm256_add_ps(_mm256_mul_ps(zr, zr), _mm256_mul_ps(zi, zi));

            // Mascara: 0xFFFFFFFF para lanes donde norma2 <= 4 (aun no escaparon)
            //          0x00000000 para lanes que ya escaparon
            __m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ);

            // Incrementar contador SOLO en lanes activos (norma2 <= 4)
            mk = _mm256_add_ps(mk, _mm256_and_ps(mask, one)); // mk[i] += mask[i] ? 1 : 0

            // Si TODOS los lanes escaparon, la mascara es todo ceros → salir
            if (_mm256_testz_ps(mask, _mm256_set1_ps(-1))) break;

            iter++;
        }

        // Volcar los 8 contadores a un array de float para asignar colores
        float d[8];
        _mm256_storeu_ps(d, mk);   // 'u' = sin requisito de alineacion de memoria

        for (int it = 0; it < 8; it++) {
            int index = (j + it) * width + i;
            if (index < width * height) {                          // guard: height no multiplo de 8
                if (d[it] < max_iteraciones)
                    pixel_buffer[index] = color_ramp[(int)(d[it]) % PALETTE_SIZE];
                else
                    pixel_buffer[index] = 0xFF000000;
            }
        }
    }
}
```

**Flujo del lazo interno — por lane:**

```
iter=1..N
  ┌─ zr,zi  →  zr² - zi² + cr  /  2·zr·zi + ci     (nueva z para los 8 lanes)
  ├─ norma2 = zr² + zi²
  ├─ mask   = norma2 <= 4  →  0xFFFFFFFF : 0x00000000
  ├─ mk    += mask & 1    →  solo avanza el contador si el lane sigue activo
  └─ si mask == 0x00..00 en todos → break (todos escaparon)
```

**Puntos clave:**

- `_mm256_set_ps(v7..v0)` carga en orden descendente: el primer argumento va al lane 7.
- La mascara de escape permite que lanes individuales "se congelen" sin interrumpir los demas — clave del paralelismo de datos.
- `_mm256_testz_ps(mask, mask)` retorna 1 si todos los bits de la mascara son 0 — salida anticipada cuando todo el bloque de 8 pixeles escapo.
- `_mm256_storeu_ps` (con `u`) no requiere alineacion de 32 bytes en `d[8]`.

---

## Bloque D — Paleta de color

### EJ-09 | `palette.h` / `palette.cpp` — `bswap32` y `color_ramp`

> **Observar:** como se invierten los bytes de un `uint32_t` para convertir entre formatos de color, y como se define la paleta ciclica.

```cpp
// Invierte el orden de los 4 bytes de un uint32_t
uint32_t bswap32(uint32_t a) {
    return ((a & 0xFF000000) >> 24)   // byte 3 (RR) → byte 0
         | ((a & 0x00FF0000) >>  8)   // byte 2 (GG) → byte 1
         | ((a & 0x0000FF00) <<  8)   // byte 1 (BB) → byte 2
         | ((a & 0x000000FF) << 24);  // byte 0 (AA) → byte 3
}

// 16 colores: azul claro → rojo oscuro
// Los literales son 0xRRGGBBAA; bswap32 los convierte al layout RGBA en memoria
std::vector<uint32_t> color_ramp = {
    bswap32(0xFF1010FF),   // azul claro
    bswap32(0xEF1019FF),
    // ... 14 entradas mas ...
    bswap32(0x1919A4FF)    // rojo oscuro
};
```

**Por que bswap32:**

```
Literal en codigo (big-endian conceptual):  0xRR GG BB AA
En memoria x86 little-endian:               AA BB GG RR  (byte bajo primero)
sf::Texture::update espera en memoria:      RR GG BB AA
=> bswap32 invierte para que coincidan.
```

**Puntos clave:**

- `PALETTE_SIZE = 16` — con `iter % 16`, los colores se repiten ciclicamente si `max_iteraciones > 16`.
- El color de puntos que no escapan es `0xFF000000` (negro opaco), codificado directamente sin pasar por la paleta.

---

## Bloque E — DLL C++ para Java

### EJ-10 | `03.fractal-dll/fractal_simd.cpp` — Exportacion ABI

> **Observar:** como `extern "C"` y `__stdcall` definen una firma estable que JNR-FFI puede encontrar por nombre.

```cpp
// Constante interna de la DLL (autonoma, no viene por extern)
std::complex<double> c(-0.7, 0.27015);

extern "C"   // Desactiva el name mangling de C++ → simbolo exportado: "julia_simd"
__stdcall    // Convencion Windows: el CALLEE limpia el stack
void julia_simd(double x_min, double y_min, double x_max, double y_max,
                uint32_t width, uint32_t height,
                int max_iteraciones, uint32_t* pixel_buffer) {
    // Mismo algoritmo SIMD AVX2 que en la app C++
    // Diferencia: max_iteraciones llega como parametro (no como global extern)
}
```

**Diferencias respecto a `001.fractal-Julia/fractal_simd.cpp`:**

| Aspecto           | App C++                  | DLL C++                 |
| ----------------- | ------------------------ | ----------------------- |
| `max_iteraciones` | variable global `extern` | parametro de la funcion |
| `c`               | variable global `extern` | constante local         |
| Firma             | sin decoradores          | `extern "C" __stdcall`  |

```powershell
# Verificar que el simbolo esta exportado correctamente
objdump -p .\03.fractal-dll\build\Release\libfractal-dll.dll | Select-String "julia_simd"
```

**Puntos clave:**

- Sin `extern "C"`, el compilador C++ manglea el nombre a algo como `_Z10julia_simdddddjiPj` — JNR-FFI no puede encontrarlo por el nombre simple.
- `__stdcall` es obligatorio en DLLs Windows de interop; omitirlo corrompe el stack.

---

### EJ-11 | `03.fractal-dll/CMakeLists.txt` — Build de la DLL

> **Observar:** la diferencia entre `add_executable` y `add_library(SHARED)`.

```cmake
cmake_minimum_required(VERSION 3.16)
project(FractalJulia)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")  # sin esto, __m256 no compila

find_package(fmt CONFIG REQUIRED)

# SHARED = DLL en Windows, .so en Linux
add_library(fractal-dll SHARED
    palette.cpp
    fractal_simd.cpp
)

target_link_libraries(fractal-dll PRIVATE fmt::fmt)
# Resultado: libfractal-dll.dll en build/Release/
```

**Puntos clave:**

- `SHARED` produce un `.dll` con tabla de exportaciones — es lo que Java carga con `LibraryLoader`.
- MinGW agrega el prefijo `lib` automaticamente: target `fractal-dll` → archivo `libfractal-dll.dll`.

---

## Bloque F — App Java

### EJ-12 | `FractalParams.java` — Configuracion centralizada

> **Observar:** como Java centraliza todos los parametros en constantes estaticas, incluyendo el formato de color para OpenGL.

```java
public class FractalParams {
    public static final int WIDTH  = 1600;
    public static final int HEIGHT = 900;
    public static int maxIteraciones = 10;    // no final: se modifica en tiempo real

    public static final double xMin = -1.5, xMax = 1.5;
    public static final double yMin = -1.0, yMax = 1.0;
    public static final double cReal = -0.7, cImag = 0.27015;
    public static final int PALETTE_SIZE = 16;

    // Formato int para glTexImage2D(GL_RGBA, GL_UNSIGNED_BYTE) en little-endian:
    //   byte[0]=R, byte[1]=G, byte[2]=B, byte[3]=A en memoria
    //   => int Java = 0xAABBGGRR
    // Ejemplo: azul puro (R=0, G=0, B=255, A=255) → 0xFFFF0000
    public static final int[] colorRamp = {
        0xFFFF0000,   // azul puro
        0xFFEE0000,
        // ... 14 entradas ...
        0xFF080000    // casi negro
    };

    // Paleta alternativa para modo Threads (naranja/azul)
    public static final int[] colorRampThreads = {
        0xFF00A5FF, /* ... */ 0xFF000210
    };
}
```

**Puntos clave:**

- `maxIteraciones` es `static` pero no `final` — el callback de teclado de GLFW lo modifica directamente.
- El formato `0xAABBGGRR` en Java para `GL_RGBA GL_UNSIGNED_BYTE` difiere de C++ (que usa `bswap32`) porque Java interpreta el `int` en little-endian de memoria.
- Dos paletas permiten distinguir visualmente CPU serial (azul) vs Threads (naranja).

---

### EJ-13 | `FractalCpu.java` — CPU serial y multihilo

> **Observar:** la equivalencia con Serial 2 de C++, y como la particion de filas elimina condiciones de carrera sin usar `synchronized`.

#### CPU serial

```java
int acotado_2(double x, double y) {
    int iter = 1;
    double zr = x, zi = y;
    while (iter < FractalParams.maxIteraciones && (zr*zr + zi*zi) < 4.0) {
        double dr = zr*zr - zi*zi + FractalParams.cReal;
        double di = 2.0*zr*zi  + FractalParams.cImag;
        zr = dr; zi = di;
        iter++;
    }
    if (iter < FractalParams.maxIteraciones)
        return FractalParams.colorRamp[iter % FractalParams.PALETTE_SIZE];
    return 0xFF000000;
}
```

#### Multihilo por filas

```java
void julia_threads_2(...) {
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    int cores       = Runtime.getRuntime().availableProcessors();
    int threadCount = Math.min(cores, height);  // no mas hilos que filas
    Thread[] threads = new Thread[threadCount];

    int rowsPerThread = height / threadCount;
    int remainder     = height % threadCount;   // filas sobrantes
    int startRow = 0;

    for (int t = 0; t < threadCount; t++) {
        int extra    = (t < remainder) ? 1 : 0;   // distribuir sobrante en los primeros hilos
        int endRow   = startRow + rowsPerThread + extra;
        int rowStart = startRow;                   // captura final para la lambda

        threads[t] = new Thread(() -> {
            // Indices exclusivos: este hilo solo escribe filas [rowStart, endRow)
            for (int j = rowStart; j < endRow; j++) {
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

    // Barrera: esperar a todos antes de que paint() use pixelBuffer
    for (Thread thread : threads) {
        try { thread.join(); } catch (InterruptedException e) { /* ... */ }
    }
}
```

**Particion de filas (ejemplo con 4 hilos, height=10):**

```
hilo 0: filas 0, 1, 2  (rowsPerThread=2 + extra=1)
hilo 1: filas 3, 4, 5  (rowsPerThread=2 + extra=1)
hilo 2: filas 6, 7     (rowsPerThread=2 + extra=0)
hilo 3: filas 8, 9     (rowsPerThread=2 + extra=0)
```

**Puntos clave:**

- `rowStart` se captura en variable final local antes de crear la lambda — `startRow` muta en el bucle y no puede capturarse directamente.
- `thread.join()` es la barrera: garantiza que todo el buffer este escrito antes del `glTexImage2D`.

---

### EJ-14 | `FractalDll.java` + `FractalSimd.java` — Puente JVM/C++

> **Observar:** como JNR-FFI mapea una interfaz Java a una funcion C++ exportada, y por que el buffer debe ser directo y con orden de bytes nativo.

```java
// FractalDll.java — interfaz que JNR-FFI implementa como proxy en tiempo de ejecucion
public interface FractalDll {
    String LIBRARY_NAME = "libfractal-dll";

    // JNR-FFI busca el simbolo "julia_simd" en la DLL y genera el marshalling automaticamente
    FractalDll INSTANCE = LibraryLoader.create(FractalDll.class).load(LIBRARY_NAME);

    void julia_simd(double x_min, double y_min, double x_max, double y_max,
                    int width, int height,
                    int max_iteraciones, Buffer pixel_buffer);
}
```

```java
// FractalSimd.java — gestiona el buffer compartido y ejecuta la llamada
public class FractalSimd {
    ByteBuffer pixelBuffer;

    public FractalSimd() {
        // allocateDirect: memoria fuera del heap Java, con direccion fija (el GC no la mueve)
        // nativeOrder():  little-endian en Windows, mismo orden que escribe la DLL C++
        this.pixelBuffer = ByteBuffer
            .allocateDirect(FractalParams.WIDTH * FractalParams.HEIGHT * Integer.BYTES)
            .order(ByteOrder.nativeOrder());
    }

    public void juliaSimd() {
        FractalDll.INSTANCE.julia_simd(
            FractalParams.xMin, FractalParams.yMin,
            FractalParams.xMax, FractalParams.yMax,
            FractalParams.WIDTH, FractalParams.HEIGHT,
            FractalParams.maxIteraciones,
            pixelBuffer    // C++ recibe un uint32_t* apuntando a esta memoria directa
        );
        pixelBuffer.rewind();  // resetear position=0 para lectura posterior
    }
}
```

**Puntos clave:**

- `allocateDirect` es obligatorio: los buffers de heap Java pueden ser movidos por el GC; un buffer directo tiene direccion fija que C++ puede usar de forma segura.
- Sin `ByteOrder.nativeOrder()`, el buffer usa big-endian por defecto y los colores se interpretarian incorrectamente.
- `LibraryLoader.load("libfractal-dll")` busca `libfractal-dll.dll` en el PATH o en el directorio de trabajo.

---

### EJ-15 | `FractalMain.java` — Orquestacion y render OpenGL

> **Observar:** el ciclo completo Java: mode dispatch → buffer copy → `glTexImage2D` → quad → overlay.

#### Seleccion de modo y subida de textura (`paint`)

```java
private void paint() {
    int fps = fpsCounter.update();
    pixelBuffer.clear();   // reset position=0, limit=capacity

    if (modo == 1) {
        fractalCpu.julia_serial_2(...);
        pixelBuffer.put(fractalCpu.pixelBuffer);            // int[] → IntBuffer
    } else if (modo == 2) {
        fractalSimd.juliaSimd();
        pixelBuffer.put(fractalSimd.pixelBuffer.asIntBuffer());  // ByteBuffer → IntBuffer
    } else if (modo == 3) {
        fractalCpu.julia_threads_2(...);
        pixelBuffer.put(fractalCpu.pixelBuffer);
    }
    pixelBuffer.flip();    // limit=position, position=0 → listo para leer

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        FractalParams.WIDTH, FractalParams.HEIGHT,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        pixelBuffer         // CPU → GPU
    );

    // Quad de pantalla completa con coordenadas de textura
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2d(-1,-1);
        glTexCoord2f(0,1); glVertex2d(-1, 1);
        glTexCoord2f(1,1); glVertex2d( 1, 1);
        glTexCoord2f(1,0); glVertex2d( 1,-1);
    glEnd();

    drawOverlay(fps);  // HUD como segunda textura (Java2D → BufferedImage → glTexImage2D)
}
```

#### Callbacks de teclado (registrados en `init`)

```java
glfwSetKeyCallback(window, (win, key, scancode, action, mods) -> {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(win, true);
    if (key == GLFW_KEY_UP     && action == GLFW_RELEASE) FractalParams.maxIteraciones += 10;
    if (key == GLFW_KEY_DOWN   && action == GLFW_RELEASE) {
        FractalParams.maxIteraciones -= 10;
        if (FractalParams.maxIteraciones < 0) FractalParams.maxIteraciones = 10;
    }
    if (key == GLFW_KEY_1 && action == GLFW_RELEASE) modo = 1;  // Java CPU
    if (key == GLFW_KEY_2 && action == GLFW_RELEASE) modo = 2;  // C++ SIMD
    if (key == GLFW_KEY_3 && action == GLFW_RELEASE) modo = 3;  // Java Threads
});
```

**Puntos clave:**

- `pixelBuffer.flip()` es obligatorio antes de `glTexImage2D` — sin flip, `position` apunta al final y OpenGL lee 0 bytes.
- El quad `(-1,-1) → (1,1)` con proyeccion `glOrtho(-1,1,-1,1,-1,1)` hace que la textura cubra exactamente toda la ventana.
- El overlay crea un `BufferedImage` con Java2D cada frame — es simple pero costoso; en produccion se usaria una biblioteca de texto para OpenGL.

---

## Bloque G — Build

### EJ-16 | `001.fractal-Julia/CMakeLists.txt` — App C++

```cmake
cmake_minimum_required(VERSION 3.16)
project(FractalJulia)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

find_package(fmt  CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(FractalJulia
    main.cpp fractal_serial.cpp palette.cpp fractal_simd.cpp)

target_link_libraries(FractalJulia PRIVATE
    fmt::fmt SFML::System SFML::Graphics SFML::Window)
```

```powershell
cmake -S 001.fractal-Julia -B 001.fractal-Julia/build -G "Ninja Multi-Config"
cmake --build 001.fractal-Julia/build --config Release
.\001.fractal-Julia\build\Release\FractalJulia.exe
```

---

### EJ-17 | `03.fractal-dll/CMakeLists.txt` — DLL C++

```cmake
cmake_minimum_required(VERSION 3.16)
project(FractalJulia)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

find_package(fmt CONFIG REQUIRED)

add_library(fractal-dll SHARED palette.cpp fractal_simd.cpp)
target_link_libraries(fractal-dll PRIVATE fmt::fmt)
```

```powershell
cmake -S 03.fractal-dll -B 03.fractal-dll/build -G "Ninja Multi-Config"
cmake --build 03.fractal-dll/build --config Release
objdump -p .\03.fractal-dll\build\Release\libfractal-dll.dll | Select-String "julia_simd"
```

---

### EJ-18 | `02.fractal-java/build.gradle.kts` — App Java

```kotlin
dependencies {
    implementation("org.lwjgl", "lwjgl")
    implementation("org.lwjgl", "lwjgl-glfw")
    implementation("org.lwjgl", "lwjgl-opengl")
    runtimeOnly("org.lwjgl:lwjgl::natives-windows")
    runtimeOnly("org.lwjgl:lwjgl-glfw::natives-windows")
    runtimeOnly("org.lwjgl:lwjgl-opengl::natives-windows")
    implementation("com.github.jnr:jnr-ffi:2.2.19")
}
```

```powershell
cd 02.fractal-java
.\gradlew run     # compilar + ejecutar
.\gradlew build   # solo compilar
```

> Antes de ejecutar: copiar `libfractal-dll.dll` desde `03.fractal-dll/build/Release/` al directorio de trabajo de la app Java o al PATH.

### 1) Mapa rapido del repositorio

- `ejemplo01` y `ejemplo02`: base de SFML y entorno de desarrollo.
- `001.fractal-Julia`: app C++ completa (Serial 1, Serial 2, SIMD).
- `03.fractal-dll`: kernel SIMD C++ exportado como DLL.
- `02.fractal-java`: app Java (CPU, Threads, DLL SIMD por FFI).

### 2) Importaciones clave por lenguaje

#### C++ (en fractal y ejemplos)

```cpp
#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <omp.h>
#include <complex>
#include <immintrin.h>
```

Que significa cada una:

- `SFML/Graphics.hpp`: ventana, textura, texto, eventos.
- `fmt/core.h`: mensajes formateados.
- `omp.h`: consulta de hilos OpenMP.
- `complex`: version matematica con `std::complex`.
- `immintrin.h`: intrinsecos AVX2 SIMD.

#### Java (en fractal)

```java
import org.lwjgl.glfw.*;
import org.lwjgl.opengl.*;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import jnr.ffi.LibraryLoader;
```

Que significa cada una:

- `lwjgl.glfw/opengl`: ventana y render OpenGL.
- `ByteBuffer`/`IntBuffer`: buffers para texturas y FFI.
- `LibraryLoader`: carga de DLL nativa.

### 3) Variables y parametros que gobiernan todo

#### C++ (`001.fractal-Julia/main.cpp`)

```cpp
#define WIDTH 1920
#define HEIGHT 1080

int max_iteraciones = 10;
double x_min = -1.5, x_max = 1.5;
double y_min = -1.0, y_max = 1.0;
std::complex<double> c(-0.7, 0.27015);
```

#### Java (`FractalParams.java`)

```java
public static final int WIDTH = 1600;
public static final int HEIGHT = 900;
public static int maxIteraciones = 10;
public static final double cReal = -0.7;
public static final double cImag = 0.27015;
```

Regla mental:

- Si cambian estos parametros, cambia el comportamiento visual y rendimiento del fractal.

### 4) Configuracion de build que habilita capacidades

#### CMake C++

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)
```

- `-mavx2` habilita las intrucciones SIMD usadas en el codigo.

#### Gradle Java

```kotlin
implementation("org.lwjgl", "lwjgl-opengl")
implementation("org.lwjgl", "lwjgl-glfw")
implementation("com.github.jnr:jnr-ffi:2.2.19")
```

- LWJGL habilita UI OpenGL.
- `jnr-ffi` habilita llamada a DLL.

### 5) Flujo general de un frame (contexto de ejecucion)

```text
Input teclado -> seleccionar modo -> calcular fractal -> subir textura -> mostrar frame
```

Memoria rapida:

- Todos los ejercicios viven dentro de este flujo.

## Bloque A - Base del entorno

### Ejercicio 1 - `ejemplo01`: ventana + eventos + OpenMP

Archivo: `ejemplo01/main.cpp`

Chunk clave:

```cpp
int max_threads = omp_get_max_threads();
fmt::print("Sistema listo. Hilos disponibles: {}\n", max_threads);

while (window.isOpen()) {
	while (const std::optional event = window.pollEvent()) {
		if (event->is<sf::Event::Closed>()) {
			window.close();
		}
	}
}
```

Explicacion simple:

- Verifica que el entorno de paralelismo esta disponible.
- Verifica el loop de eventos moderno de SFML 3.

Memoria rapida:

- `ejemplo01` = prueba minima de plataforma.

### Ejercicio 2 - `ejemplo02`: render con texto

Archivo: `ejemplo02/main.cpp`

Chunk clave:

```cpp
const sf::Font font("arial.ttf");
sf::Text text(font, "Hello SFML", 50);

window.clear(sf::Color(30, 30, 30));
window.draw(text);
window.display();
```

Explicacion simple:

- Agrega recursos graficos (fuente/texto) sobre la base del ejercicio 1.

Memoria rapida:

- `ejemplo02` = ventana + contenido visual.

## Bloque B - Fractal C++

### Ejercicio 3 - Selector de modo en tiempo real

Archivo: `001.fractal-Julia/main.cpp`

Chunk clave:

```cpp
if (r_type == runtime_type::SERIAL_1) {
	julia_serial_1(...);
} else if (r_type == runtime_type::SERIAL_2){
	julia_serial_2(...);
} else if (r_type == runtime_type::SIMD) {
	julia_simd(...);
}
```

Explicacion simple:

- El `main` decide que implementacion corre por frame.

Memoria rapida:

- `main.cpp` orquesta, no optimiza matematica.

### Ejercicio 4 - Serial 1 (`std::complex`)

Archivo: `001.fractal-Julia/fractal_serial.cpp`

Chunk clave:

```cpp
std::complex<double> z = z0;
while (iter < max_iteraciones && std::abs(z) < 2.0) {
	z = z * z + c;
	iter++;
}
```

Explicacion simple:

- Es la version mas cercana a la formula matematica.

Memoria rapida:

- Serial 1 = claridad primero.

### Ejercicio 5 - Serial 2 (`zr`/`zi`)

Archivo: `001.fractal-Julia/fractal_serial.cpp`

Chunk clave:

```cpp
double dr = zr*zr - zi*zi + c.real();
double di = 2.0*zr*zi + c.imag();
zr = dr;
zi = di;
```

Explicacion simple:

- Mismo algoritmo, sin `std::complex`, mas directo para CPU.

Memoria rapida:

- Serial 2 = misma logica, menos overhead.

### Ejercicio 6 - SIMD AVX2 (8 pixeles)

Archivo: `001.fractal-Julia/fractal_simd.cpp`

Chunk clave:

```cpp
for (int j = 0; j < height; j += 8) {
	__m256 my = _mm256_set_ps(j+7, j+6, j+5, j+4, j+3, j+2, j+1, j+0);
	__m256 mask = _mm256_cmp_ps(norma2, max_norma, _CMP_LE_OQ);
	if (_mm256_testz_ps(mask, _mm256_set1_ps(-1))) {
		break;
	}
}
```

Explicacion simple:

- Una instruccion procesa 8 datos en paralelo.
- La mascara controla que lanes siguen iterando.

Memoria rapida:

- SIMD = 8 caminos de datos con una sola secuencia de instrucciones.

### Ejercicio 7 - Paleta y bytes de color

Archivos: `001.fractal-Julia/palette.cpp`, `001.fractal-Julia/palette.h`

Chunk clave:

```cpp
uint32_t bswap32(uint32_t a) {
	return ((a & 0xFF000000) >> 24) |
		   ((a & 0x00FF0000) >> 8)  |
		   ((a & 0x0000FF00) << 8)  |
		   ((a & 0x000000FF) << 24);
}
```

Explicacion simple:

- Corrige el orden de bytes para que la textura pinte colores correctos.

Memoria rapida:

- Bug visual frecuente: byte order, no la formula de Julia.

### Ejercicio 8 - Build C++

Archivo: `001.fractal-Julia/CMakeLists.txt`

Chunk clave:

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)
```

Explicacion simple:

- `-mavx2` habilita el objetivo SIMD del proyecto.

Memoria rapida:

- Sin bandera AVX2, la ruta SIMD pierde sentido.

## Bloque C - DLL C++ para Java

### Ejercicio 9 - Exportar `julia_simd`

Archivo: `03.fractal-dll/fractal_simd.cpp`

Chunk clave:

```cpp
extern "C" __stdcall
void julia_simd(double x_min, double y_min, double x_max, double y_max,
	uint32_t width, uint32_t height,
	int max_iteraciones, uint32_t* pixel_buffer) {
	// ...
}
```

Explicacion simple:

- Define una firma estable para que Java pueda invocar codigo nativo.

Memoria rapida:

- ABI correcto = Java encuentra y ejecuta la funcion.

### Ejercicio 10 - Build y simbolos DLL

Archivos: `03.fractal-dll/CMakeLists.txt`, `03.fractal-dll/comandos.txt`

Chunk clave:

```cmake
add_library(fractal-dll SHARED
	palette.cpp
	fractal_simd.cpp
)
```

Explicacion simple:

- Crea la DLL y luego `objdump` permite validar exportaciones.

Memoria rapida:

- Si falla FFI, primero confirmar simbolo exportado.

## Bloque D - Java: CPU, Threads y FFI

### Ejercicio 11 - Flujo principal y modo de ejecucion

Archivo: `02.fractal-java/src/main/java/com/programacion/paralela/FractalMain.java`

Chunk clave:

```java
if (modo == 1) {
	fractalCpu.julia_serial_2(...);
} else if (modo == 2) {
	fractalSimd.juliaSimd();
} else if (modo == 3) {
	fractalCpu.julia_threads_2(...);
}
```

Explicacion simple:

- Java selecciona el backend de calculo y luego renderiza textura OpenGL.

Memoria rapida:

- `FractalMain` = control + render.

### Ejercicio 12 - Threads por filas

Archivo: `02.fractal-java/src/main/java/com/programacion/paralela/FractalCpu.java`

Chunk clave:

```java
int cores = Runtime.getRuntime().availableProcessors();
int threadCount = Math.min(cores, height);

threads[t] = new Thread(() -> {
	for (int j = rowStart; j < endRow; j++) {
		for (int i = 0; i < width; i++) {
			pixelBuffer[j * width + i] = acotado_threads_2(x, y);
		}
	}
});
```

Explicacion simple:

- Cada hilo recibe un rango de filas y escribe indices exclusivos.

Memoria rapida:

- Particion por filas evita carreras de escritura.

### Ejercicio 13 - Llamada nativa desde Java

Archivos: `02.fractal-java/src/main/java/com/programacion/paralela/FractalDll.java`, `02.fractal-java/src/main/java/com/programacion/paralela/FractalSimd.java`

Chunk clave:

```java
FractalDll INSTANCE = LibraryLoader.create(FractalDll.class).load("libfractal-dll");

this.pixelBuffer = ByteBuffer.allocateDirect(...)
							 .order(ByteOrder.nativeOrder());

FractalDll.INSTANCE.julia_simd(..., pixelBuffer);
```

Explicacion simple:

- Java usa FFI para invocar el kernel SIMD C++ sobre buffer nativo directo.

Memoria rapida:

- `allocateDirect + nativeOrder` = datos correctos al cruzar JVM/C++.

### Ejercicio 14 - Configuracion de build Java

Archivo: `02.fractal-java/build.gradle.kts`

Chunk clave:

```kotlin
dependencies {
	implementation("org.lwjgl", "lwjgl-glfw")
	implementation("org.lwjgl", "lwjgl-opengl")
	implementation("com.github.jnr:jnr-ffi:2.2.19")
}
```

Explicacion simple:

- LWJGL cubre ventana/OpenGL; `jnr-ffi` cubre interoperabilidad con la DLL.

Memoria rapida:

- Sin estas dependencias no hay UI OpenGL ni puente nativo.
