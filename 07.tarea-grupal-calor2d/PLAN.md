# Plan de arquitectura — Trabajo Grupal 1: Ecuación de Calor 2D

> **Cómo usar este documento:** es la guía de diseño para que el grupo escriba el código.
> Cada sección dice QUÉ construir, POR QUÉ, y QUÉ ejemplo de clase usar como referencia
> (referencia = leerlo, entender el patrón y reescribirlo para este problema; nunca
> copiar bloques — ver sección 9, la política del enunciado prohíbe copias >30 líneas).

---

## 1. Estructura de archivos propuesta

Misma organización que `001.fractal-Julia` (un `.cpp/.h` por backend + `main.cpp` con la UI):

```
07.tarea-grupal-calor2d/
├── CMakeLists.txt
├── arial.ttf              (copiar junto al ejecutable o cargar por ruta)
├── main.cpp               UI SFML + bucle principal + selección de backend
├── args.h / args.cpp      struct Args + parseo de línea de comandos
├── heat_serial.h/.cpp     backend 1: serial de referencia
├── heat_simd.h/.cpp       backend 2: AVX2 con intrínsecas
├── heat_openmp.h/.cpp     backend 3: OpenMP
├── heat_mpi.h/.cpp        backend 4: MPI (franjas + filas fantasma)
├── palette.h/.cpp         paleta de colores (misma técnica que el fractal)
└── PLAN.md                este documento
```

**Regla de oro del diseño:** cada backend expone UNA función con la misma firma, que
avanza UNA iteración y devuelve el residuo L2 de esa iteración:

```
double heat_step_serial(const double* u, double* u_new, int nx, int ny, double r);
double heat_step_simd  (const double* u, double* u_new, int nx, int ny, double r);
double heat_step_openmp(const double* u, double* u_new, int nx, int ny, double r);
// MPI tiene su propio flujo (sección 6) porque el campo está repartido entre ranks
```

Así `main.cpp` no sabe nada de paralelismo: solo llama a la función activa, hace
`std::swap(u, u_new)` y renderiza. Es el mismo patrón del `switch` de `r_type` en
`001.fractal-Julia/main.cpp`.

---

## 2. La matemática (lo que hay que entender antes de programar)

### 2.1 Esquema de 5 puntos (Jacobi explícito)

```
u_new[i,j] = u[i,j] + r * (u[i+1,j] + u[i-1,j] + u[i,j+1] + u[i,j-1] - 4*u[i,j])
donde r = alpha * dt / h^2
```

- El campo se guarda como `std::vector<double>` **aplanado por filas**:
  `u[j*nx + i]` (misma convención `M[i*n+j]` de `Practice/MPI/Practice/04_producto_exterior.cpp`).
- Se recorren solo los puntos **interiores**: `i = 1..nx-2`, `j = 1..ny-2`.
  Los bordes nunca se tocan (Dirichlet fijo).
- Es **Jacobi**: se lee de `u` y se escribe en `u_new` (dos buffers + swap).
  Esto es lo que hace el problema paralelizable: ninguna celda de la iteración k+1
  depende de otra celda de la iteración k+1.

### 2.2 Condiciones de contorno e inicialización

- Fila superior (j = 0 si la fila 0 es "arriba" en pantalla): `u = 100.0` (fuente caliente).
- Otros tres bordes: `u = 0.0`. Interior inicial: `0.0`.
- Inicializar **ambos** buffers (`u` y `u_new`) con los bordes, porque tras el swap
  el borde caliente debe seguir presente en el buffer que se lee.

### 2.3 Estabilidad (CFL)

`r = alpha*dt/h^2` debe cumplir `r <= 0.25`. Con los defaults (Nx=1024, dt=5e-7,
alpha=0.25): `r ≈ 0.131` ✓. **Validar en `args.cpp` al arrancar** e imprimir un error
con `fmt::print` si no se cumple — es un detalle de "código limpio" que suma en la rúbrica.

Para desarrollo usar `--nx 128 --ny 128 --dt 1e-5` (el propio enunciado lo sugiere):
la difusión se ve a simple vista en pocas iteraciones.

### 2.4 Residuo L2 — el "patrón de reducción" de la rúbrica

```
residuo = sqrt( suma((u_new - u)^2) / (Nx*Ny) )
```

La suma de cuadrados es una **reducción**, y la rúbrica (criterio 5) evalúa
explícitamente el patrón de reducción. Cada backend la implementa distinto — ese
contraste es material de oro para el informe:

| Backend | Cómo reduce |
|---|---|
| serial | acumulador escalar en el bucle |
| simd   | acumulador vectorial `__m256d` + suma horizontal al final |
| openmp | `reduction(+:suma)` en el pragma |
| mpi    | suma local por rank + `MPI_Allreduce(..., MPI_SUM, ...)` |

### 2.5 MFLOPS estimados

Por celda interior y por iteración: 7 flops del stencil (4 sumas + 1 resta + 2 mult)
+ 3 flops del residuo (resta + mult + suma) = **10 flops**.

```
MFLOPS = 10 * (nx-2) * (ny-2) * iteraciones_medidas / tiempo_segundos / 1e6
```

Medir tiempo con `std::chrono::steady_clock` (u `omp_get_wtime` / `MPI_Wtime` según
backend) SOLO alrededor del solver, nunca del render.

---

## 3. `struct Args` y flags de línea de comandos

El enunciado exige alinearse con `struct Args`. Propuesta (defaults exactos de la
tabla 2.1 del PDF):

```
struct Args {
    int    nx      = 1024;     // --nx
    int    ny      = 1024;     // --ny
    double lx      = 1.0;      // --lx
    double ly      = 1.0;      // --ly
    double alpha   = 0.25;     // --alpha
    double dt      = 5.0e-7;   // --dt
    int    iter    = 1000;     // --iter   (máximo de iteraciones)
    double tol     = 1.0e-4;   // --tol    (parada por residuo)
    std::string backend = "serial";  // --backend serial|simd|openmp|mpi
};
```

Parseo simple con un bucle sobre `argv` (`if (arg == "--nx") nx = std::stoi(next)`),
sin librerías externas. Derivados: `h = lx/nx`, `r = alpha*dt/(h*h)`.

---

## 4. Render SFML (`main.cpp`) — referencia: `001.fractal-Julia/main.cpp`

El main del fractal ya tiene TODOS los elementos que pide el enunciado; hay que
**reescribirlos** para este problema (estructura similar es inevitable y esperada —
"el código entregado debe seguir la estructura del ejemplo realizado en clases" —
pero cada línea la escriben ustedes):

- Ventana `sf::VideoMode({1600, 900})`, título "Ecuación de Calor 2D".
- `sf::Texture` de tamaño **Nx×Ny** (una celda = un texel) y `sf::Sprite` **escalado**
  a 1600×900 con `sprite.setScale({1600.f/nx, 900.f/ny})`.
- `uint32_t* pixel_buffer = new uint32_t[nx*ny]` → `texture.update(...)` cada frame.
- Overlay superior izq. (`arial.ttf`, como pide 4.1):
  `"Backend: {} | Iter: {}/{} | Residuo L2: {:.3e} | MFLOPS: {:.1f} | FPS: {}"`.
- Overlay inferior izq. con el menú:
  `"[1] Serial [2] SIMD [3] OpenMP [Espacio] Continuo/Pausa [S] Paso [R] Reset [Up/Down] iter"`.
- Eventos SFML 3: `while (const std::optional event = window.pollEvent())`,
  `event->is<sf::Event::Closed>()`, `getIf<sf::Event::KeyReleased>()` — igual que el fractal.

**Modos de ejecución** (requisito "paso a paso o modo continuo"):

```
bool en_pausa = true;          // arrancar pausado
bool un_paso  = false;         // [S] pone true un frame
// en el bucle: if (!en_pausa || un_paso) { avanzar 1..K iteraciones; un_paso = false; }
```

Sugerencia: en modo continuo avanzar K=10 iteraciones por frame (con Nx=1024 una
iteración serial tarda ~ms; 1 iter/frame se vería lentísimo). Parar cuando
`iter >= max_iter || residuo < tol` y mostrarlo en el overlay.

### 4.1 Mapa de calor con la paleta del fractal

El enunciado pide usar la técnica de `palette.h/cpp`. Reescribir la paleta con una
rampa **frío→caliente** (azul→rojo; pueden generarla en rampgenerator.com como hizo
el profe, con más entradas, p.ej. 32, para gradiente suave) y mapear temperatura→color:

```
// u en [0,100] -> índice en [0, PALETTE_SIZE-1]
int idx = (int)(u[j*nx+i] / 100.0 * (PALETTE_SIZE-1));
idx = std::clamp(idx, 0, PALETTE_SIZE-1);
pixel_buffer[j*nx+i] = color_ramp[idx];
```

Mantener el truco `bswap32` de `palette.cpp` para el orden de canales RGBA de SFML.

---

## 5. Backends de cómputo compartido

### 5.1 `heat_serial.cpp` (3 pts)

Doble bucle sobre interiores + acumulador del residuo. Es la referencia de
correctitud y la base de tiempos del informe. Escribirlo PRIMERO y validarlo
visualmente (el calor debe "bajar" desde el borde superior de forma simétrica).

**Test de correctitud para todos los demás backends:** con los mismos args, el
residuo de la iteración N debe coincidir con el serial (SIMD puede diferir en el
último decimal por reasociación de sumas — comentarlo en el informe, es un punto fino
que demuestra comprensión).

### 5.2 `heat_simd.cpp` (3 pts) — referencia: `001.fractal-Julia/fractal_simd.cpp`

**Diferencia clave con el fractal:** el fractal usa `__m256` (8 floats); aquí el campo
es `double`, así que se usa **`__m256d` (4 doubles)** y los intrínsecos terminan en
`_pd`: `_mm256_loadu_pd`, `_mm256_add_pd`, `_mm256_mul_pd`, `_mm256_storeu_pd`.
Esto ya garantiza que no es copia del fractal — el kernel es otro.

Esquema del kernel (por fila interior j, procesando 4 celdas a la vez):

```
r_vec    = _mm256_set1_pd(r);  cuatro = _mm256_set1_pd(4.0);
para i = 1; i+3 <= nx-2; i += 4:
    centro = loadu(&u[j*nx + i])
    izq    = loadu(&u[j*nx + i - 1])      // desalineado: por eso loadu
    der    = loadu(&u[j*nx + i + 1])
    arriba = loadu(&u[(j-1)*nx + i])
    abajo  = loadu(&u[(j+1)*nx + i])
    vecinos = izq + der + arriba + abajo - 4*centro
    nuevo   = centro + r * vecinos
    storeu(&u_new[j*nx + i], nuevo)
    diff = nuevo - centro
    acum = acum + diff*diff               // residuo vectorial
// resto: bucle escalar para las (nx-2) % 4 celdas finales
// al final: suma horizontal de acum (storeu a double[4] y sumar, como el
// _mm256_storeu_ps(d, mk) del fractal)
```

Nota de rendimiento para el informe: el stencil es *memory-bound* (10 flops vs
5 lecturas + 1 escritura de 8 bytes), por eso el speedup SIMD será menor que el
teórico ×4 — explicarlo vale más que el número.

### 5.3 `heat_openmp.cpp` (4 pts) — referencia: `001.fractal-Julia/fractal_openmp.cpp`

Implementar **las dos variantes** que se vieron en clase (el fractal tiene ambas):

1. **Regiones paralelas manuales** (`#pragma omp parallel` + reparto por bloques de
   filas con `ceil(filas/threads)` — mismo patrón de `julia_openmp_regiones`).
   El residuo local por hilo se acumula con `#pragma omp atomic` o en un arreglo
   por hilo. El enunciado marca "paralelización manual", así que esta variante es
   la principal.
2. **`#pragma omp parallel for reduction(+:suma)`** sobre `j` — más corta y muestra
   la reducción idiomática.

`OMP_NUM_THREADS` configurable (requisito explícito): no fijar nada en el código;
mostrar `omp_get_num_threads()` en el overlay (mismo truco del fractal:
`#pragma omp parallel` + `#pragma omp master` al inicio). Para el informe:
`$env:OMP_NUM_THREADS = 1, 2, 4, 8` y tabular.

### 5.4 Selección de backend en caliente

Igual que el fractal: `enum` + teclas 1/2/3 cambian la función que se llama. El flag
`--backend` fija el inicial. MPI **no** entra al enum (se decide al lanzar con
`mpiexec`, sección 6).

---

## 6. Backend MPI (6 pts — el más valioso) — referencias: `06.fractal-mpi/main.cpp` + `Practice/MPI`

### 6.1 Descomposición: franjas de filas con padding (patrón del repo)

```
filas_int = ny - 2;                              // solo interiores se reparten
bloque    = std::ceil(filas_int * 1.0 / nprocs); // patrón N*1.0 del repo
padding   = bloque * nprocs - filas_int;
mis_filas = (rank == nprocs-1) ? bloque - padding : bloque;
fila_ini  = 1 + rank * bloque;                   // fila global inicial
```

Cada rank guarda su franja **con 2 filas fantasma** (una arriba, una abajo):
`std::vector<double> u_local((bloque + 2) * nx)`. La fila local 0 y la fila local
`mis_filas+1` son copias de las filas frontera de los ranks vecinos (o el borde
Dirichlet si el vecino no existe).

### 6.2 Una iteración MPI (aquí caen los dos tipos de comunicación de la rúbrica)

```
1. INTERCAMBIO DE FANTASMAS (punto a punto):
   - con el vecino de arriba (rank-1, si existe):
       MPI_Send(mi primera fila real)   / MPI_Recv(su última fila -> mi fantasma 0)
   - con el vecino de abajo (rank+1, si existe):
       MPI_Send(mi última fila real)    / MPI_Recv(su primera fila -> mi fantasma inferior)
   - rank 0 no tiene vecino arriba: su fantasma superior es el borde u=100 (fijo)
   - último rank: fantasma inferior = borde u=0 (fijo)

   ⚠ DEADLOCK: si todos hacen Send antes de Recv con mensajes grandes, se bloquea.
   Solución estilo del curso: ordenar por paridad (ranks pares envían primero y
   luego reciben; impares al revés) o usar MPI_Sendrecv (una sola llamada, sin
   riesgo). Documentar la elección en el informe — demuestra comprensión.

2. CÓMPUTO: stencil sobre mis_filas (idéntico al serial, sobre u_local).

3. RESIDUO (colectiva #1):
   suma_local -> MPI_Allreduce(&suma_local, &suma_global, 1, MPI_DOUBLE, MPI_SUM, ...)
   Allreduce y no Reduce: TODOS los ranks necesitan el residuo para decidir si parar
   (si solo rank 0 lo supiera, los demás no sabrían cuándo salir del bucle).

4. CONTROL DE UI (colectiva #2):
   rank 0 difunde {running, en_pausa/pasos} con MPI_Bcast — mismo patrón del
   vector dummy de 06.fractal-mpi/main.cpp.

5. RECOLECCIÓN PARA RENDER (colectiva #3):
   MPI_Gather(mi franja sin fantasmas, bloque*nx, MPI_DOUBLE, campo_global, ...)
   — el padding hace los bloques iguales, así que Gather directo funciona
   (misma razón por la que 03_colectivas.cpp rellena con ceros). Rank 0 descarta
   las filas de padding al pintar, igual que imprimir_matriz en
   04_producto_exterior.cpp descarta filas.
   Optimización opcional: recolectar solo cada K iteraciones (el render no
   necesita cada paso) — medir el efecto para el informe.
```

Con esto el criterio 4 queda cubierto de sobra: **punto a punto** (fantasmas) +
**colectivas** (Allreduce, Bcast, Gather), cada una con una razón de ser clara.

### 6.3 Estructura maestro/trabajador (igual que `06.fractal-mpi`)

- `rank 0`: abre la ventana SFML, procesa eventos, participa del cómputo con su
  propia franja, recolecta y pinta.
- `ranks > 0`: bucle `while(running)`: Bcast control → fantasmas → stencil →
  Allreduce → Gather. Salen cuando `running == 0`.
- Ejecutar: `mpiexec -n 4 build\Release\calor2d.exe --backend mpi`
  (cargar antes `cd C:\oneAPI && setvars`).

### 6.4 Timing MPI para el informe

Patrón exacto de `Practice/MPI/08.ejemplos-cap8-mpi/05_reduce_min_max_avg.cpp`:
`MPI_Barrier` antes de empezar, `MPI_Wtime()` alrededor del bucle del solver, y
tres `MPI_Reduce` (MAX/MIN/SUM) para reportar min/max/promedio de los ranks.
El tiempo que se reporta como "tiempo MPI" es el **máximo** (el más lento manda).

---

## 7. CMakeLists.txt

Combinar los dos CMake del repo (fractal + mpi). Un solo ejecutable:

```cmake
cmake_minimum_required(VERSION 3.16)
project(Calor2D)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -fopenmp -Wall")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

set(MPI_ROOT "c:/oneAPI/mpi/latest")            # patrón de 05.ejemplo-mpi
# ... target_include_directories(${MPI_ROOT}/include)
# ... target_link_libraries(... ${MPI_ROOT}/lib/impi.lib)
```

- `-Wall` desde el día 1: la rúbrica pide "sin warnings".
- Compilar SIEMPRE también en `Release` (`cmake --build build --config Release`):
  los tiempos del informe se miden en Release, nunca en Debug.
- Configurar con el toolchain de vcpkg (`x64-mingw-dynamic`) como los demás proyectos.

**Comandos de verificación** (los del punto 5 del enunciado — probarlos en limpio
antes de entregar):

```powershell
set PATH=c:/tools/mingw64/bin;%PATH%
cmake -B build -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=... (igual que CLAUDE.md)
cmake --build build --config Release
./build/Release/calor2d.exe --backend serial --nx 128 --ny 128 --dt 1e-5
cd C:\oneAPI && setvars
mpiexec -n 4 C:\tools\prog-paralela\07.tarea-grupal-calor2d\build\Release\calor2d.exe --backend mpi
```

---

## 8. Informe + screencast (4 pts) — metodología de medición

### 8.1 Protocolo de medición (fijarlo ANTES de medir)

- Trabajo fijo: `--iter 500 --tol 0` (tol=0 desactiva la parada temprana para que
  todos los backends hagan exactamente el mismo trabajo).
- Tamaños: al menos 512², 1024², 2048² (para ver cómo escala con el problema).
- Release, sin render en la medición (o midiendo solo el solver), 3-5 corridas y
  reportar la mediana. Cerrar programas pesados de fondo.
- Anotar el hardware: CPU, núcleos físicos/lógicos, RAM (va en el informe).

### 8.2 Tablas y gráficas mínimas

1. Tiempo por backend (serial / simd / openmp 1,2,4,8 hilos / mpi 1,2,4,8 procesos).
2. **Speedup** `S(p) = T_serial / T(p)` — gráfica S vs p con la diagonal ideal.
3. **Eficiencia** `E(p) = S(p)/p` — comentar por qué cae (memoria compartida
   saturada en OpenMP; costo de fantasmas + Gather en MPI).
4. MFLOPS por backend.
5. Verificación de correctitud: residuo en la iteración 500 idéntico entre backends.

### 8.3 Qué explicar (esto separa un 4/4 de un 2/4)

- Por qué Jacobi y no Gauss-Seidel (paralelizable sin dependencias).
- El patrón de **reducción** en los 4 backends (tabla de la sección 2.4).
- Por qué SIMD no da ×4 (memory-bound) y OpenMP no escala linealmente (ancho de banda).
- El intercambio de fantasmas: costo de comunicación vs cómputo por iteración
  (relación superficie/volumen: comunicación O(nx), cómputo O(nx·bloque)).

### 8.4 Screencast ≤ 3 min (guion sugerido)

0:00-0:30 compilación limpia sin warnings · 0:30-1:30 demo visual: difusión con
128², paso a paso, cambio serial→simd→openmp con las métricas del overlay cambiando ·
1:30-2:30 `mpiexec -n 4` funcionando · 2:30-3:00 gráfica de speedup y conclusión.

---

## 9. Cumplimiento de la política de originalidad (sección 7 del enunciado)

- **Escriban cada archivo desde cero** mirando los ejemplos como referencia de
  patrón, no de texto. El problema es distinto (stencil vs fractal), así que el
  código naturalmente diverge; el riesgo real de copia >30 líneas está en
  `main.cpp` (bucle SFML) y `palette.cpp` — reescribirlos con sus propias
  decisiones (paleta térmica propia, controles propios, textos propios).
- **Commits de todos los integrantes** desde el inicio (git es la evidencia natural
  de trabajo propio e incremental). Nada de un único commit gigante al final.
- **Cada integrante debe poder explicar cualquier parte** — repartirse los backends
  para escribir, pero hacer revisión cruzada en pareja antes de integrar.
- Si usan IA como apoyo (dudas puntuales, revisión), que sea eso: apoyo. El código
  entregado lo escriben y lo entienden ustedes.

## 10. Reparto sugerido y cronograma (2 semanas)

| Días | Hito | Responsable sugerido |
|---|---|---|
| 1-2 | Esqueleto: CMake + Args + serial + render básico (sin overlay) | Integrante A |
| 3-4 | Overlays, paleta térmica, pausa/paso/reset — **hito: demo serial completa** | A + B |
| 4-6 | Backend OpenMP (2 variantes) + verificación vs serial | Integrante B |
| 5-8 | Backend SIMD `__m256d` + verificación vs serial | Integrante C |
| 6-10 | Backend MPI (fantasmas → Allreduce → Gather → UI en rank 0) | Integrante D (+A) |
| 11-12 | Mediciones (protocolo 8.1) en una sola máquina, tablas y gráficas | Todos |
| 13 | Informe + screencast | Todos |
| 14 | Compilación limpia en máquina "virgen", revisión de warnings/código muerto, entrega | Todos |

**Orden de dependencias:** serial es prerequisito de todo (es la referencia de
correctitud y la base del speedup). SIMD/OpenMP/MPI son independientes entre sí y
se pueden hacer en paralelo (apropiado para la asignatura 😄).
