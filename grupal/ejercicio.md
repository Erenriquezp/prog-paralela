# Documentación y Análisis de Resultados: Simulación de la Ecuación de Calor 2D

Este documento describe la arquitectura, fundamentos matemáticos y el análisis de resultados de los proyectos para la resolución numérica en paralelo de la **Ecuación de Calor 2D** mediante diferencias finitas (esquema de 5 puntos y Jacobi explícito).

---

## 1. Descripción del Proyecto
El objetivo es resolver la transferencia de calor por conducción en un dominio bidimensional rectangular $\Omega = (0, L_x) \times (0, L_y)$ a lo largo del tiempo, empleando cuatro enfoques de programación (Serial, SIMD AVX2, OpenMP y MPI). El programa renderiza el mapa de temperaturas en una ventana SFML ($1600 \times 900$ px) utilizando una rampa térmica y muestra un overlay con estadísticas en tiempo real (Backend, Iteración, Residuo $L_2$, MFLOPS y FPS).

Para respetar la imposibilidad de integrar de forma nativa la ejecución MPI distribuida con los esquemas de memoria compartida multinúcleo en un mismo binario, el trabajo se dividió en dos proyectos independientes:
1. **[`grupal_ecuacionDeCalor`](file:///c:/tools/prog-paralela/grupal/grupal_ecuacionDeCalor)**: Contiene los backends de memoria compartida (Serial, SIMD AVX2 y OpenMP Regiones).
2. **[`ecuacionDeCalor-mpi`](file:///c:/tools/prog-paralela/grupal/ecuacionDeCalor-mpi)**: Contiene el backend distribuido (MPI) con soporte para comunicación punto a punto y colectiva.

---

## 2. Fundamentos Matemáticos y Físicos

### 2.1 Ecuación de Difusión del Calor
La variación temporal de la temperatura $u(x, y, t)$ está dada por la ecuación diferencial parcial:
$$\frac{\partial u}{\partial t} = \alpha \left( \frac{\partial^2 u}{\partial x^2} + \frac{\partial^2 u}{\partial y^2} \right)$$
Donde $\alpha = 0.25$ es el coeficiente de difusividad térmica del material.

### 2.2 Discretización Espacio-Temporal (Stencil de 5 Puntos)
Discretizando el dominio mediante una grilla uniforme de $N_x \times N_y$ celdas con paso espacial $h = L_x/N_x = L_y/N_y$ y paso temporal $\Delta t$, obtenemos el esquema explícito de Jacobi (stencil de 5 puntos):
$$u_{\text{new}}[i,j] = u[i,j] + r \cdot \Big( u[i+1,j] + u[i-1,j] + u[i,j+1] + u[i,j-1] - 4 \cdot u[i,j] \Big)$$
Donde el parámetro de estabilidad adimensional es $r = \frac{\alpha \Delta t}{h^2}$.

### 2.3 Estabilidad Numérica (Condición CFL)
Para evitar que la simulación diverja numéricamente, se debe cumplir estrictamente la condición de Courant-Friedrichs-Lewy (CFL) en 2D:
$$r \le 0.25 \implies \Delta t \le \frac{h^2}{4\alpha}$$
* Con los parámetros por defecto ($N_x=N_y=1024$, $L_x=L_y=1.0$, $\alpha=0.25$ y $\Delta t=5.0 \times 10^{-7}$):
  $$h \approx 9.77 \times 10^{-4} \implies r \approx 0.131 \le 0.25 \quad (\text{Estable}) \checkmark$$

### 2.4 Criterio de Parada por Convergencia
La simulación se detiene al alcanzar las 1000 iteraciones o de manera anticipada si la norma del residuo global $L_2$ cae por debajo de la tolerancia $\text{tol} = 1.0 \times 10^{-4}$:
$$\|r\|_2 = \sqrt{\frac{\sum_{j,i} (u_{\text{new}}[i,j] - u_{\text{old}}[i,j])^2}{N_x \cdot N_y}} < 1.0 \times 10^{-4}$$

---

## 3. Arquitectura del Software y Paralelismo

### 3.1 Backend 1: Serial de Referencia
* **Archivo**: [`calor_serial.cpp`](file:///c:/tools/prog-paralela/grupal/grupal_ecuacionDeCalor/calor_serial.cpp)
* **Descripción**: Implementación básica y secuencial que sirve como base de correctitud para validar los resultados de los demás backends y medir el factor de aceleración (*Speedup*).

### 3.2 Backend 2: Vectorización SIMD (AVX2)
* **Archivo**: [`calor_simd.cpp`](file:///c:/tools/prog-paralela/grupal/grupal_ecuacionDeCalor/calor_simd.cpp)
* **Descripción**: Utiliza registros e instrucciones intrínsecas AVX de 256 bits para procesar 8 datos de tipo `float` en paralelo (`_mm256_loadu_ps`, `_mm256_add_ps`, etc.). Reduce el residuo de manera vectorial acumulando sumas parciales que se consolidan mediante suma horizontal al final de cada fila. Maneja celdas remanentes al final de la fila con un bucle escalar de limpieza.

### 3.3 Backend 3: Multinúcleo (OpenMP Regiones)
* **Archivo**: [`calor_openmp.cpp`](file:///c:/tools/prog-paralela/grupal/grupal_ecuacionDeCalor/calor_openmp.cpp)
* **Descripción**: Implementa paralelismo de memoria compartida manual mediante `#pragma omp parallel`. Divide las filas de la grilla de cómputo equitativamente entre los hilos del runtime. Incluye una optimización matemática para repartir el residuo aritmético de filas de forma uniforme, evitando que el último hilo concentre todo el remanente. Utiliza `reduction(+:suma_diferencias)` para evitar colisiones en la reducción del residuo.

### 3.4 Backend 4: Distribuido (MPI)
* **Archivos**: [`calor_mpi.cpp`](file:///c:/tools/prog-paralela/grupal/ecuacionDeCalor-mpi/calor_mpi.cpp) y [`main.cpp`](file:///c:/tools/prog-paralela/grupal/ecuacionDeCalor-mpi/main.cpp)
* **Descripción**: Descompone el dominio de cálculo horizontalmente en franjas de filas (`delta = Ny / nproc`) distribuidas entre procesos.
  * **Intercambio de Halos (Punto a Punto)**: Cada proceso mantiene dos filas fantasma (una superior y otra inferior) que se intercambian bidireccionalmente con sus vecinos inmediatos utilizando la directiva segura `MPI_Sendrecv` para prevenir interbloqueos (*deadlocks*).
  * **Sincronización de Controles (Colectiva)**: El Rank 0 (GUI maestro) difunde mediante `MPI_Bcast` los estados de interacción del teclado (`iteracion_objetivo` y `running`).
  * **Reducción del Residuo (Colectiva)**: Los procesos reportan sus sumas cuadráticas locales y consolidan el residuo global $L_2$ síncronamente vía `MPI_Allreduce` para validar la parada por convergencia de forma coordinada.
  * **Recolección de Píxeles**: Los procesos esclavos envían su búfer de renderizado (`pixel_buffer`) al Rank 0 a través de `MPI_Send`, y el maestro los recibe con `MPI_Recv` para reconstruir la imagen global.

---

## 4. Análisis de Resultados y Simulación

### 4.1 Comportamiento Físico con la Grilla por Defecto ($1024 \times 1024$)
Al ejecutar el programa con la grilla de $1024 \times 1024$ y avanzar hasta el límite de 1000 iteraciones, se observa que **el calor casi no se propaga**, apareciendo únicamente como una delgadísima franja roja en el borde superior:
* **Explicación**: El paso de tiempo de estabilidad $\Delta t = 5.0 \times 10^{-7}\text{ s}$ es tan pequeño que 1000 iteraciones equivalen a apenas $0.0005$ segundos físicos. En este tiempo, el calor no tiene posibilidad de difundirse en el medio (el tiempo característico de difusión total $t_c$ es de $4.0$ segundos). 
* El residuo $L_2$ al paso 1000 es de aproximadamente **`0.003277`**, un valor superior al límite de tolerancia de parada ($0.0001$), lo que corrobora que la simulación sigue activa y no ha alcanzado estabilidad.

### 4.2 Optimización Visual con Grilla de $128 \times 128$
Para observar la propagación del calor de manera clara en pocas iteraciones, se aumentó el paso espacial reduciendo la grilla a $128 \times 128$ y se incrementó el paso temporal a $\Delta t = 1.0 \times 10^{-5}\text{ s}$ (respetando la estabilidad CFL).
* **Resultado**: La simulación avanza 20 veces más rápido en tiempo físico por iteración. El calor desciende de forma curva y simétrica por el dominio en pasos visibles.
* **Convergencia**: Al correr la simulación de manera continua, el gradiente de calor se estabiliza. Conforme se aproxima al **Estado Estacionario** (donde el flujo de calor de entrada se equilibra con la pérdida por los bordes fríos), el Residuo $L_2$ desciende rápidamente por debajo de $1.0 \times 10^{-4}$. Al cumplirse esto, el programa detiene las iteraciones automáticamente y muestra el indicador **`(CONVERGIDO)`** en la pantalla del Rank 0.

### 4.3 Análisis de Rendimiento (MFLOPS y Cuellos de Botella)
* **SIMD**: Ofrece una mejora de rendimiento de punto flotante teórica de hasta 8x al procesar 8 datos en paralelo. Sin embargo, en la práctica el factor de aceleración es menor debido a que el stencil de 5 puntos es un algoritmo acotado por el ancho de banda de memoria (*Memory-Bound*): realiza pocas operaciones aritméticas (7 FLOPs de stencil + 3 FLOPs de residuo) en comparación con las numerosas lecturas desalineadas de memoria requeridas por celda.
* **MPI**: Muestra tasas de procesamiento muy elevadas en paralelo. No obstante, al recolectar e interpolar los búferes de píxeles píxel por píxel mediante software en la CPU del Rank 0, se genera un cuello de botella que limita los FPS en resoluciones altas.

---

## 5. Guía de Ejecución y Verificación

### 5.1 Compilación de los Proyectos (Windows)
Asegúrese de contar con GCC (con soporte C++20 y OpenMP), Intel MPI y SFML instalados en su sistema.

1. **Compilar el proyecto de memoria compartida (Serial/OpenMP/SIMD)**:
   ```powershell
   cd c:\tools\prog-paralela\grupal\grupal_ecuacionDeCalor
   cmake -B build -G "MinGW Makefiles"
   cmake --build build --config Release
   ```

2. **Compilar el proyecto distribuido (MPI)**:
   *(Asegúrese de cargar las variables de entorno de Intel MPI llamando a `setvars` de oneAPI)*
   ```powershell
   cd c:\tools\prog-paralela\grupal\ecuacionDeCalor-mpi
   cmake -B build -G "MinGW Makefiles"
   cmake --build build --config Release
   ```

### 5.2 Comandos de Ejecución
* **Ejecutar Versión Compartida**:
  ```powershell
  .\build\EcuacionCalor.exe
  ```
* **Ejecutar Versión Distribuida (con 4 procesos locales)**:
  ```powershell
  mpiexec -n 4 .\build\calor-mpi.exe
  ```