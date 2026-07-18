# Lógica y Matemática de Fractales en C++

Este documento detalla la lógica algebraica, las condiciones de escape/convergencia y los algoritmos paralelos en MPI para los tres fractales clave del curso: **Julia**, **Newton** y **Burning Ship**.

---

## 1. Mapeo de Píxeles al Plano Complejo
Para renderizar cualquier fractal en una pantalla de resolución $W \times H$ (píxeles) sobre un dominio complejo $[x_{\text{min}}, x_{\text{max}}] \times [y_{\text{min}}, y_{\text{max}}]$:

*   **Paso de discretización ($dx$, $dy$):**
    $$dx = \frac{x_{\text{max}} - x_{\text{min}}}{W - 1}, \quad dy = \frac{y_{\text{max}} - y_{\text{min}}}{H - 1}$$
*   **Mapeo de coordenada $(i, j)$ (donde $i \in [0, W-1]$ y $j \in [0, H-1]$):**
    $$x = x_{\text{min}} + i \cdot dx$$
    $$y = y_{\text{max}} - j \cdot dy \quad (\text{se resta } j \text{ porque el eje Y en pantalla va hacia abajo})$$

---

## 2. Fractal de Julia ($z_{n+1} = z_n^2 + c$)

### Lógica Matemática
*   **Ecuación:** $z_{n+1} = z_n^2 + c$, con $c$ constante compleja fija y $z_0 = x + i y$.
*   **Desglose algebraico:**
    Si $z_n = z_r + i z_i$ y $c = c_r + i c_i$:
    $$z_n^2 = (z_r + i z_i)^2 = (z_r^2 - z_i^2) + i(2 z_r z_i)$$
    $$z_{n+1} = (z_r^2 - z_i^2 + c_r) + i(2 z_r z_i + c_i)$$
*   **Condición de escape:** Si la norma $|z_n| > 2$ (equivalente a $|z_n|^2 > 4$), la órbita diverge al infinito.
    $$z_r^2 + z_i^2 > 4.0$$

### Implementación en Código (Aritmética Real)
```cpp
double zr = x;
double zi = y;
while (iter < max_iteraciones && (zr*zr + zi*zi) < 4.0) {
    double temp_r = zr*zr - zi*zi + c.real(); // o c_real
    double temp_i = 2.0*zr*zi + c.imag(); // o c_imag
    zr = temp_r;
    zi = temp_i;
    iter++;
}
```

---

## 3. Fractal de Newton ($z^3 - 1 = 0$)

### Lógica Matemática
Aplica el método de Newton-Raphson en el plano complejo para encontrar las raíces de $f(z) = z^3 - 1 = 0$.
*   **Fórmula de iteración:**
    $$z_{n+1} = z_n - \frac{f(z_n)}{f'(z_n)} = z_n - \frac{z_n^3 - 1}{3 z_n^2}$$
*   **Desglose algebraico:**
    Sea $z = z_r + i z_i$:
    1. **Numerador ($f(z) = z^3 - 1$):**
       $$z^2 = (z_r^2 - z_i^2) + i(2 z_r z_i)$$
       $$z^3 = (z_r + i z_i)(z_r^2 - z_i^2 + i(2 z_r z_i)) = (z_r^3 - 3 z_r z_i^2) + i(3 z_r^2 z_i - z_i^3)$$
       $$nr = z_r^3 - 3 z_r z_i^2 - 1, \quad ni = 3 z_r^2 z_i - z_i^3$$
    2. **Denominador ($f'(z) = 3 z^2$):**
       $$dr = 3(z_r^2 - z_i^2), \quad di = 6 z_r z_i$$
    3. **División compleja ($\frac{nr + i \cdot ni}{dr + i \cdot di}$):**
       Multiplicando por el conjugado del denominador:
       $$\frac{nr + i \cdot ni}{dr + i \cdot di} = \frac{(nr \cdot dr + ni \cdot di) + i(ni \cdot dr - nr \cdot di)}{dr^2 + di^2}$$
       $$fr = \frac{nr \cdot dr + ni \cdot di}{dr^2 + di^2}, \quad fi = \frac{ni \cdot dr - nr \cdot di}{dr^2 + di^2}$$
    4. **Actualización:**
       $$z_{r, n+1} = z_{r, n} - fr$$
       $$z_{i, n+1} = z_{i, n} - fi$$

### Raíces y Criterio de Convergencia
Las tres raíces cúbicas de la unidad son:
$$R_0 = 1.0 + 0.0i$$
$$R_1 = -0.5 + 0.8660254i$$
$$R_2 = -0.5 - 0.8660254i$$
*   **Tolerancia ($\epsilon$):** Si la distancia euclidiana al cuadrado de $z_n$ a cualquiera de las raíces $R_k$ es menor a $\epsilon^2$ (donde $\epsilon = 10^{-4}$), el algoritmo converge a la raíz $k$.
    $$(z_r - R_{k, r})^2 + (z_i - R_{k, i})^2 < \epsilon^2$$

### Implementación en Código
```cpp
double zr = cx;
double zi = cy;
int iter = 0;
while (iter < max_iteraciones) {
    double r2 = zr*zr + zi*zi;
    if (r2 > 4.0 || r2 < 1e-12) return 0xFF000000; // Evita indeterminación o escape excesivo

    double zr2 = zr * zr;
    double zi2 = zi * zi;
    
    // Numerador: nr + i*ni = z^3 - 1
    double nr = zr * zr2 - 3.0 * zr * zi2 - 1.0;
    double ni = 3.0 * zr2 * zi - zi * zi2;

    // Denominador: dr + i*di = 3*z^2
    double dr = 3.0 * (zr2 - zi2);
    double di = 6.0 * zr * zi;

    double denom = dr * dr + di * di;
    if (denom < 1e-12) return 0xFF000000;

    // División: fr + i*fi = (nr + i*ni) / (dr + i*di)
    double fr = (nr * dr + ni * di) / denom;
    double fi = (ni * dr - nr * di) / denom;

    zr -= fr;
    zi -= fi;

    // Comprobar convergencia a una de las 3 raíces
    for (int k = 0; k < 3; ++k) {
        double dist2 = (zr - roots_r[k])*(zr - roots_r[k]) + (zi - roots_i[k])*(zi - roots_i[k]);
        if (dist2 < EPS * EPS) {
            out_iter = iter;
            return color_ramp[(k * 5 + iter) % PALETTE_SIZE];
        }
    }
    iter++;
}
```

---

## 4. Fractal de Burning Ship

### Lógica Matemática
*   **Ecuación:** $z_{n+1} = (|\text{Re}(z_n)| + i |\text{Im}(z_n)|)^2 + c$, con $z_0 = 0$ y $c = cx + i cy$ (coordenada del píxel).
*   **Desglose algebraico:**
    Sea $z_n = z_r + i z_i$:
    $$ar = |z_r|, \quad ai = |z_i|$$
    $$(ar + i \cdot ai)^2 = (ar^2 - ai^2) + i(2 \cdot ar \cdot ai)$$
    $$z_{n+1} = (ar^2 - ai^2 + c_x) + i(2 \cdot ar \cdot ai + c_y)$$
*   **Condición de escape:** $|z_n|^2 > 4 \implies z_r^2 + z_i^2 > 4.0$.

### Histogramas de Iteración (Bining)
Se divide el rango de iteraciones en bins (`NBINS`) para cuantificar el esfuerzo computacional:
*   **Determinación de Bin:**
    $$\text{bin} = \text{iter} \cdot \frac{\text{NBINS}}{\text{max\_iter}}$$
    $$\text{si } \text{bin} \ge \text{NBINS} \implies \text{bin} = \text{NBINS} - 1$$
*   **Acumulación local:** `local_hist[bin]++`.

### Implementación en Código
```cpp
double zr = 0.0;
double zi = 0.0;
while (iter < max_iteraciones && (zr * zr + zi * zi) <= 4.0) {
    double ar = std::abs(zr);
    double ai = std::abs(zi);
    double next_zr = ar * ar - ai * ai + cx;
    double next_zi = 2.0 * ar * ai + cy;
    zr = next_zr;
    zi = next_zi;
    iter++;
}
```

---

## 5. Plantilla Matemática Universal para Fractales (C++)

Esta plantilla generaliza el cálculo de cualquier fractal basado en órbitas complejas mediante aritmética real pura.

```cpp
#include <cmath>
#include <cstdint>

uint32_t evaluar_pixel(double cx, double cy, int max_iter, int& out_iter) {
    // 1. Inicialización de z0
    // - Para Julia:
    double zr = cx;
    double zi = cy;
    // - Para Mandelbrot / Burning Ship:
    // double zr = 0.0;
    // double zi = 0.0;
    
    int iter = 0;
    while (iter < max_iter) {
        // 2. Condición de escape / parada
        double r2 = zr*zr + zi*zi;
        if (r2 > 4.0) { // Si escapa
            out_iter = iter;
            return color_ramp[iter % PALETTE_SIZE];
        }
        
        // 3. Fórmula matemática (adaptar según corresponda)
        double next_zr = zr*zr - zi*zi + cx;
        double next_zi = 2.0*zr*zi + cy;
        
        zr = next_zr;
        zi = next_zi;
        iter++;
    }
    
    out_iter = iter;
    return 0xFF000000; // Si no escapó (interior)
}
```

---

## 6. Consejos Prácticos para Exámenes y Paralelismo en MPI

1.  **División Equitativa de Carga (Filas):**
    *   Divide las filas de la imagen entre el número de procesos:
        `delta = std::ceil(HEIGHT * 1.0 / nprocs);`
    *   Determina los índices locales:
        `row_start = rank * delta;`
        `row_end = std::min(row_start + delta, HEIGHT);`
    *   **Padding:** Previene desbordamiento en el último proceso esclavo:
        `int padding = delta * nprocs - HEIGHT;`
        `int my_delta = (rank == nprocs - 1) ? (delta - padding) : delta;`
2.  **Sincronización mediante Broadcast:**
    *   Transmite parámetros interactivos del Rank 0 a todos los workers con `MPI_Bcast` antes de calcular cada frame:
        ```cpp
        MPI_Bcast(datos_estado.data(), size, MPI_INT, 0, MPI_COMM_WORLD);
        ```
3.  **Reducción y Recolección de Datos:**
    *   **Estadísticas Globales:** Utiliza `MPI_Reduce` para encontrar valores globales aggregados (tiempo máximo, iteraciones totales):
        ```cpp
        MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        ```
    *   **Histograma Acumulado:** Envía histogramas locales a un búfer global mediante `MPI_Gather` y redúcelos secuencialmente en el Rank 0:
        ```cpp
        MPI_Gather(local_hist, NBINS, MPI_INT, gather_buf, NBINS, MPI_INT, 0, MPI_COMM_WORLD);
        ```
