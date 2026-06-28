## 8.4 Ejemplos de paralelismo de datos

La estrategia de paralelismo de datos (data parallel), definida en la sección 1.5, es el enfoque más común en las aplicaciones paralelas. Veremos algunos ejemplos de este enfoque en esta sección. Primero, veremos un caso simple del stream triad donde no es necesaria ninguna comunicación. Luego veremos las técnicas más típicas de intercambio de ghost cells (celdas fantasma) que se usan para enlazar los dominios subdivididos distribuidos a cada proceso.

### 8.4.1 Stream triad para medir el ancho de banda en el nodo

El STREAM Triad es un código de benchmark para probar el ancho de banda, introducido en la sección 3.2.4. Esta versión usa MPI para poner más procesos a trabajar en el nodo y, posiblemente, en múltiples nodos. El propósito de tener más procesos es ver cuál es el ancho de banda máximo del nodo cuando se usan todos los procesadores. Esto da un ancho de banda objetivo al que apuntar con aplicaciones más complicadas. Como muestra el listado 8.17, el código es simple porque no se requiere comunicación entre ranks. El tiempo solo se reporta en el proceso principal. Puede ejecutar esto primero en un procesador y luego en todos los procesadores de su nodo. ¿Obtiene la aceleración (speedup) paralela completa que esperaría del aumento de procesadores? ¿Cuánto limita su aceleración el ancho de banda de memoria del sistema?

**Listado 8.17 — La versión MPI del STREAM Triad**

`StreamTriad/StreamTriad.c`

```c
 1 #include <stdio.h>
 2 #include <stdlib.h>
 3 #include <time.h>
 4 #include <mpi.h>
 5 #include "timer.h"
 6
 7 #define NTIMES 16
 8 #define STREAM_ARRAY_SIZE 80000000
 9
10 int main(int argc, char *argv[]){
11
12 MPI_Init(&argc, &argv);
13
14 int nprocs, rank;
15 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
16 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
17 int ibegin = STREAM_ARRAY_SIZE *(rank )/nprocs;
18 int iend = STREAM_ARRAY_SIZE *(rank+1)/nprocs;
19 int nsize = iend-ibegin;
20 double *a = malloc(nsize * sizeof(double));
21 double *b = malloc(nsize * sizeof(double));
22 double *c = malloc(nsize * sizeof(double));
23
24 struct timespec tstart;
25 double scalar = 3.0, time_sum = 0.0;
26 for (int i=0; i<nsize; i++) {
27 a[i] = 1.0;
28 b[i] = 2.0;
29 }
30
31 for (int k=0; k<NTIMES; k++){
32 cpu_timer_start(&tstart);
33 for (int i=0; i<nsize; i++){
34 c[i] = a[i] + scalar*b[i];
35 }
36 time_sum += cpu_timer_stop(tstart);
37 c[1]=c[2];
38 }
39
40 free(a);
41 free(b);
42 free(c);
43
44 if (rank == 0) printf("Average runtime is %lf msecs\n", time_sum/NTIMES);
45 MPI_Finalize();
46 return(0);
47 }
```

Anotaciones del listado:

- Línea 8 (`STREAM_ARRAY_SIZE`): lo bastante grande como para forzar el uso de la memoria principal.
- Líneas 25–29 (`scalar`, inicialización): inicializa los datos y los arreglos.
- Líneas 33–35 (bucle interno `c[i] = a[i] + scalar*b[i]`): el bucle del stream triad.
- Línea 37 (`c[1]=c[2]`): evita que el compilador optimice y elimine el bucle.

### 8.4.2 Intercambios de ghost cells en una malla bidimensional (2D)

Las ghost cells (celdas fantasma) son el mecanismo que usamos para enlazar las mallas en procesadores adyacentes. Se usan para almacenar en caché valores de procesadores adyacentes, de modo que se necesiten menos comunicaciones. La técnica de ghost cells es, por sí sola, el método más importante para habilitar el paralelismo de memoria distribuida en MPI.

Hablemos un poco sobre la terminología de los halos y las ghost cells. Incluso antes de la era del procesamiento paralelo, a menudo se usaba una región de celdas que rodeaba la malla para implementar condiciones de frontera (boundary conditions). Estas condiciones de frontera podían ser reflectivas, de entrada (inflow), de salida (outflow) o periódicas. Por eficiencia, los programadores querían evitar las sentencias `if` en el bucle computacional principal. Para ello, añadían celdas alrededor de la malla y las establecían con valores apropiados antes del bucle computacional principal. Estas celdas tenían la apariencia de un halo, así que el nombre se quedó. Las celdas de halo (halo cells) son cualquier conjunto de celdas que rodean una malla computacional, independientemente de su propósito. Un halo de frontera de dominio (domain-boundary halo) es entonces un conjunto de halo cells usadas para imponer un conjunto específico de condiciones de frontera.

Una vez que las aplicaciones se paralelizaron, se añadió una región exterior de celdas similar para contener valores de las mallas vecinas. Estas celdas no son celdas reales, sino que solo existen como una ayuda para reducir los costos de comunicación. Como no son reales, pronto recibieron el nombre de ghost cells (celdas fantasma). Los datos reales de una ghost cell están en el procesador adyacente, y la copia local es solo un valor fantasma (ghost value). Las ghost cells también se parecen a halos y también se denominan halo cells. Las actualizaciones o intercambios de ghost cells (ghost cell updates o exchanges) se refieren a la actualización de las ghost cells y solo se necesitan para ejecuciones paralelas de múltiples procesos, cuando se necesitan actualizaciones de valores reales de procesos adyacentes.

Las condiciones de frontera deben realizarse tanto para ejecuciones seriales como paralelas. Existe confusión porque estas operaciones a menudo se denominan actualizaciones de halo (halo updates), aunque no está claro qué se quiere decir exactamente. En nuestra terminología, las actualizaciones de halo se refieren tanto a las actualizaciones de la frontera del dominio como a las actualizaciones de ghost cells. Para optimizar la comunicación de MPI, solo necesitamos fijarnos en las actualizaciones o intercambios de ghost cells y dejar de lado, por ahora, los cálculos de las condiciones de frontera.

Veamos ahora cómo configurar las ghost cells para los bordes de la malla local en cada proceso y realizar la comunicación entre los subdominios. Al usar ghost cells, las comunicaciones necesarias se agrupan en un número menor de comunicaciones que si se hiciera una única comunicación cada vez que se necesita el valor de una celda de otro proceso. Esta es la técnica más común para hacer que el enfoque de paralelismo de datos tenga un buen rendimiento. En las implementaciones de las actualizaciones de ghost cells, demostraremos el uso de la rutina `MPI_Pack` y cargaremos un buffer de comunicación con una asignación de arreglo celda por celda simple. En secciones posteriores, también veremos cómo realizar la misma comunicación con tipos de datos de MPI, usando las llamadas de topología de MPI para la configuración y la comunicación.

Una vez que implementamos las actualizaciones de ghost cells en un código de paralelismo de datos, la mayor parte de la comunicación necesaria queda gestionada. Esto aísla el código que proporciona el paralelismo en una pequeña sección de la aplicación. Esta pequeña sección del código es importante de optimizar para la eficiencia paralela. Veamos algunas implementaciones de esta funcionalidad, comenzando con la configuración en el listado 8.18 y el trabajo realizado por los bucles de stencil en el listado 8.19. Quizás quiera ver el código completo en el directorio `GhostExchange/GhostExchange_Pack` del código de ejemplo del capítulo en <https://github.com/EssentialsOfParallelComputing/Chapter8>.

**Listado 8.18 — Configuración para los intercambios de ghost cells en una malla 2D**

`GhostExchange/GhostExchange_Pack/GhostExchange.cc`

```cpp
30 int imax = 2000, jmax = 2000;
31 int nprocx = 0, nprocy = 0;
32 int nhalo = 2, corners = 0;
33 int do_timing;
   ....
40 int xcoord = rank%nprocx;
41 int ycoord = rank/nprocx;
42
43 int nleft = (xcoord > 0 ) ? rank - 1 : MPI_PROC_NULL;
44 int nrght = (xcoord < nprocx-1) ? rank + 1 : MPI_PROC_NULL;
45 int nbot = (ycoord > 0 ) ? rank - nprocx : MPI_PROC_NULL;
46 int ntop = (ycoord < nprocy-1) ? rank + nprocx : MPI_PROC_NULL;
47
48 int ibegin = imax *(xcoord )/nprocx;
49 int iend = imax *(xcoord+1)/nprocx;
50 int isize = iend - ibegin;
51 int jbegin = jmax *(ycoord )/nprocy;
52 int jend = jmax *(ycoord+1)/nprocy;
53 int jsize = jend - jbegin;
```

Anotaciones del listado:

- Línea 30 (`imax`, `jmax`): configuración de entrada: `-i <imax> -j <jmax>` son los tamaños de la cuadrícula.
- Línea 31 (`nprocx`, `nprocy`): `-x <nprocx> -y <nprocy>` son el número de procesos en las direcciones x e y.
- Línea 32 (`nhalo`, `corners`): `-h <nhalo>` es el número de celdas de halo y `-c` incluye las celdas de las esquinas.
- Línea 33 (`do_timing`): `-t do_timing` sincroniza la medición de tiempos.
- Líneas 40–41 (`xcoord`, `ycoord`): coordenadas `xcoord` e `ycoord` de los procesos. El índice de fila es el que varía más rápido.
- Líneas 43–46 (`nleft`, `nrght`, `nbot`, `ntop`): rank vecino de cada proceso, para la comunicación con los vecinos.
- Líneas 48–53 (`ibegin` … `jsize`): tamaño del dominio computacional de cada proceso y los índices global de inicio y fin.

Hacemos la asignación de memoria para el tamaño local más espacio para los halos en cada proceso. Para hacer la indexación un poco más simple, desplazamos la indexación de memoria para que empiece en `-nhalo` y termine en `isize+nhalo`. Las celdas reales son entonces siempre desde `0` hasta `isize-1`, independientemente del ancho del halo.

Las siguientes líneas muestran una llamada a un `malloc2D` especial con dos argumentos adicionales que desplazan el direccionamiento del arreglo, de modo que la parte real del arreglo va desde `0,0` hasta `jsize,isize`. Esto se hace con algo de aritmética de punteros que mueve la ubicación inicial de cada puntero.

```cpp
64 double** x = malloc2D(jsize+2*nhalo, isize+2*nhalo, nhalo, nhalo);
65 double** xnew = malloc2D(jsize+2*nhalo, isize+2*nhalo, nhalo, nhalo);
```

Usamos el cálculo de stencil simple del operador de desenfoque (blur) introducido en la figura 1.10 para proporcionar el trabajo. Muchas aplicaciones tienen cálculos mucho más complejos que toman mucho más tiempo. El siguiente listado muestra los bucles del cálculo de stencil.

**Listado 8.19 — El trabajo se realiza en un bucle de iteración de stencil**

`GhostExchange/GhostExchange_Pack/GhostExchange.cc`

```cpp
 91 for (int iter = 0; iter < 1000; iter++){
 92 cpu_timer_start(&tstart_stencil);
 93
 94 for (int j = 0; j < jsize; j++){
 95 for (int i = 0; i < isize; i++){
 96 xnew[j][i]= (x[j][i] + x[j][i-1] + x[j][i+1] + x[j-1][i] + x[j+1][i])/5.0;
 97 }
 98 }
 99
100 SWAP_PTR(xnew, x, xtmp);
101
102 stencil_time += cpu_timer_stop(tstart_stencil);
103
104 boundarycondition_update(x, nhalo, jsize, isize, nleft, nrght, nbot, ntop);
105 ghostcell_update(x, nhalo, corners, jsize, isize, nleft, nrght, nbot, ntop);
106 }
```

Anotaciones del listado:

- Línea 91 (`for iter`): bucle de iteración.
- Líneas 94–98 (doble bucle de stencil): cálculo de stencil.
- Línea 100 (`SWAP_PTR`): intercambio de punteros para los arreglos `x` antiguo y nuevo.
- Línea 105 (`ghostcell_update`): la llamada de actualización de ghost cells refresca las ghost cells.

Ahora podemos ver el código crítico de actualización de ghost cells. La figura 8.5 muestra la operación requerida. El ancho de la región de ghost cells puede ser de una, dos o más celdas de profundidad. Las celdas de las esquinas (corner cells) también pueden ser necesarias para algunas aplicaciones. Cuatro procesos (o ranks) necesitan datos de este rank: a la izquierda, a la derecha, arriba y abajo. Cada uno de estos procesos requiere una comunicación separada y un buffer de datos separado. El ancho de la región de halo varía en diferentes aplicaciones, así como el hecho de si se necesitan las celdas de las esquinas.

La figura 8.5 muestra un ejemplo de un intercambio de ghost cells para una malla de 4 por 4 en nueve procesos, con un halo de una celda de ancho y las esquinas incluidas. Los halos de la frontera exterior se actualizan primero, y luego un intercambio de datos horizontal, una sincronización y el intercambio de datos vertical. Si no se necesitan las esquinas, los intercambios horizontal y vertical pueden hacerse al mismo tiempo. Si se desean las esquinas, es necesaria una sincronización entre los intercambios horizontal y vertical.

> **Figura 8.5** La versión con celdas de esquina de la actualización de ghost cells primero intercambia datos hacia la izquierda y la derecha (en la mitad superior de la figura), seguido de un intercambio hacia arriba y hacia abajo (en la mitad inferior de la figura). Con cuidado, el intercambio izquierda-derecha puede ser más pequeño, con solo las celdas reales más las celdas de la frontera exterior, aunque no hay ningún problema en hacerlo del tamaño vertical completo de la malla. La actualización de las celdas de frontera que rodean la malla se realiza por separado.

Una observación clave de las actualizaciones de datos de ghost cells es que, en C, los datos de las filas son contiguos, mientras que los datos de las columnas están separados por un paso (stride) que es del tamaño de la fila. Enviar valores individuales para las columnas es costoso, así que necesitamos agruparlos de alguna manera.

Puede realizar la actualización de ghost cells con MPI de varias maneras. En esta primera versión, en el listado 8.20, veremos una implementación que usa la llamada `MPI_Pack` para empaquetar los datos de las columnas. Los datos de las filas se envían con una simple llamada estándar `MPI_Isend`. El ancho de la región de ghost cells se especifica mediante la variable `nhalo`, y las esquinas pueden solicitarse con la entrada apropiada.

**Listado 8.20 — Rutina de actualización de ghost cells para una malla 2D con MPI_Pack**

`GhostExchange/GhostExchange_Pack/GhostExchange.cc`

```cpp
167 void ghostcell_update(double **x, int nhalo, int corners, int jsize, int isize,
168 int nleft, int nrght, int nbot, int ntop, int do_timing)
169 {
170 if (do_timing) MPI_Barrier(MPI_COMM_WORLD);
171
172 struct timespec tstart_ghostcell;
173 cpu_timer_start(&tstart_ghostcell);
174
175 MPI_Request request[4*nhalo];
176 MPI_Status status[4*nhalo];
177
178 int jlow=0, jhgh=jsize;
179 if (corners) {
180 if (nbot == MPI_PROC_NULL) jlow = -nhalo;
181 if (ntop == MPI_PROC_NULL) jhgh = jsize+nhalo;
182 }
183 int jnum = jhgh-jlow;
184 int bufcount = jnum*nhalo;
185 int bufsize = bufcount*sizeof(double);
186
187 double xbuf_left_send[bufcount];
188 double xbuf_rght_send[bufcount];
189 double xbuf_rght_recv[bufcount];
190 double xbuf_left_recv[bufcount];
191
192 int position_left;
193 int position_right;
194 if (nleft != MPI_PROC_NULL){
195 position_left = 0;
196 for (int j = jlow; j < jhgh; j++){
197 MPI_Pack(&x[j][0], nhalo, MPI_DOUBLE,
198 xbuf_left_send, bufsize, &position_left, MPI_COMM_WORLD);
199 }
200 }
201
202 if (nrght != MPI_PROC_NULL){
203 position_right = 0;
204 for (int j = jlow; j < jhgh; j++){
205 MPI_Pack(&x[j][isize-nhalo], nhalo, MPI_DOUBLE, xbuf_rght_send,
206 bufsize, &position_right, MPI_COMM_WORLD);
207 }
208 }
209
210 MPI_Irecv(&xbuf_rght_recv, bufsize, MPI_PACKED, nrght, 1001,
211 MPI_COMM_WORLD, &request[0]);
212 MPI_Isend(&xbuf_left_send, bufsize, MPI_PACKED, nleft, 1001,
213 MPI_COMM_WORLD, &request[1]);
214
215 MPI_Irecv(&xbuf_left_recv, bufsize, MPI_PACKED, nleft, 1002,
216 MPI_COMM_WORLD, &request[2]);
217 MPI_Isend(&xbuf_rght_send, bufsize, MPI_PACKED, nrght, 1002,
218 MPI_COMM_WORLD, &request[3]);
219 MPI_Waitall(4, request, status);
220
221 if (nrght != MPI_PROC_NULL){
222 position_right = 0;
223 for (int j = jlow; j < jhgh; j++){
224 MPI_Unpack(xbuf_rght_recv, bufsize, &position_right, &x[j][isize],
225 nhalo, MPI_DOUBLE, MPI_COMM_WORLD);
226 }
227 }
228
229 if (nleft != MPI_PROC_NULL){
230 position_left = 0;
231 for (int j = jlow; j < jhgh; j++){
232 MPI_Unpack(xbuf_left_recv, bufsize, &position_left, &x[j][-nhalo],
233 nhalo, MPI_DOUBLE, MPI_COMM_WORLD);
234 }
235 }
236
237 if (corners) {
238 bufcount = nhalo*(isize+2*nhalo);
239 MPI_Irecv(&x[jsize][-nhalo], bufcount, MPI_DOUBLE, ntop, 1001,
240 MPI_COMM_WORLD, &request[0]);
241 MPI_Isend(&x[0 ][-nhalo], bufcount, MPI_DOUBLE, nbot, 1001,
242 MPI_COMM_WORLD, &request[1]);
243
244 MPI_Irecv(&x[ -nhalo][-nhalo], bufcount, MPI_DOUBLE, nbot, 1002,
245 MPI_COMM_WORLD, &request[2]);
246 MPI_Isend(&x[jsize-nhalo][-nhalo], bufcount, MPI_DOUBLE,ntop, 1002,
247 MPI_COMM_WORLD, &request[3]);
248 MPI_Waitall(4, request, status);
249 } else {
250 for (int j = 0; j<nhalo; j++){
251 MPI_Irecv(&x[jsize+j][0], isize, MPI_DOUBLE, ntop, 1001+j*2,
252 MPI_COMM_WORLD, &request[0+j*4]);
253 MPI_Isend(&x[0+j ][0], isize, MPI_DOUBLE, nbot, 1001+j*2,
254 MPI_COMM_WORLD, &request[1+j*4]);
255
256 MPI_Irecv(&x[ -nhalo+j][0], isize, MPI_DOUBLE, nbot, 1002+j*2,
257 MPI_COMM_WORLD, &request[2+j*4]);
258 MPI_Isend(&x[jsize-nhalo+j][0], isize, MPI_DOUBLE, ntop, 1002+j*2,
259 MPI_COMM_WORLD, &request[3+j*4]);
260 }
261 MPI_Waitall(4*nhalo, request, status);
262 }
263
264 if (do_timing) MPI_Barrier(MPI_COMM_WORLD);
265
266 ghostcell_time += cpu_timer_stop(tstart_ghostcell);
267 }
```

Anotaciones del listado:

- Líneas 167–168 (firma de `ghostcell_update`): la actualización de las ghost cells desde los procesos adyacentes.
- Líneas 194–208 (bucles `MPI_Pack`): empaqueta los buffers para la actualización de ghost cells de los vecinos izquierdo y derecho.
- Líneas 210–219: comunicación para los vecinos izquierdo y derecho.
- Líneas 221–235 (bucles `MPI_Unpack`): desempaqueta los buffers para los vecinos izquierdo y derecho.
- Líneas 237–248 (bloque `if (corners)`): actualizaciones de ghost cells en un único bloque contiguo para los vecinos inferior y superior.
- Línea 248 (`MPI_Waitall`): espera a que se complete toda la comunicación.
- Líneas 250–260 (bloque `else`): actualizaciones de ghost cells fila por fila para los vecinos inferior y superior.
- Línea 261 (`MPI_Waitall`): espera a que se complete toda la comunicación.

La llamada `MPI_Pack` es particularmente útil cuando hay múltiples tipos de datos que necesitan comunicarse en la actualización de ghost cells. Los valores se empaquetan en un buffer agnóstico al tipo (type-agnostic) y luego se desempaquetan en el otro lado. La comunicación con los vecinos en la dirección vertical se realiza con datos de filas contiguos. Cuando se incluyen las esquinas, un único buffer funciona bien. Sin esquinas, se envían filas de halo individuales. Usualmente solo hay una o dos celdas de halo, así que este es un enfoque razonable.

Otra forma de cargar los buffers para la comunicación es con una asignación de arreglo (array assignment). Las asignaciones de arreglo son un buen enfoque cuando hay un único tipo de datos simple, como el tipo `float` de doble precisión usado en este ejemplo. El siguiente listado muestra el código para reemplazar los bucles de `MPI_Pack` con asignaciones de arreglo.

**Listado 8.21 — Rutina de actualización de ghost cells para una malla 2D con asignaciones de arreglo**

`GhostExchange/GhostExchange_ArrayAssign/GhostExchange.cc`

```cpp
190 int icount;
191 if (nleft != MPI_PROC_NULL){
192 icount = 0;
193 for (int j = jlow; j < jhgh; j++){
194 for (int ll = 0; ll < nhalo; ll++){
195 xbuf_left_send[icount++] = x[j][ll];
196 }
197 }
198 }
199 if (nrght != MPI_PROC_NULL){
200 icount = 0;
201 for (int j = jlow; j < jhgh; j++){
202 for (int ll = 0; ll < nhalo; ll++){
203 xbuf_rght_send[icount++] = x[j][isize-nhalo+ll];
204 }
205 }
206 }
207
208 MPI_Irecv(&xbuf_rght_recv, bufcount, MPI_DOUBLE, nrght, 1001,
209 MPI_COMM_WORLD, &request[0]);
210 MPI_Isend(&xbuf_left_send, bufcount, MPI_DOUBLE, nleft, 1001,
211 MPI_COMM_WORLD, &request[1]);
212
213 MPI_Irecv(&xbuf_left_recv, bufcount, MPI_DOUBLE, nleft, 1002,
214 MPI_COMM_WORLD, &request[2]);
215 MPI_Isend(&xbuf_rght_send, bufcount, MPI_DOUBLE, nrght, 1002,
216 MPI_COMM_WORLD, &request[3]);
217 MPI_Waitall(4, request, status);
218
219 if (nrght != MPI_PROC_NULL){
220 icount = 0;
221 for (int j = jlow; j < jhgh; j++){
222 for (int ll = 0; ll < nhalo; ll++){
223 x[j][isize+ll] = xbuf_rght_recv[icount++];
224 }
225 }
226 }
227 if (nleft != MPI_PROC_NULL){
228 icount = 0;
229 for (int j = jlow; j < jhgh; j++){
230 for (int ll = 0; ll < nhalo; ll++){
231 x[j][-nhalo+ll] = xbuf_left_recv[icount++];
232 }
233 }
234 }
```

Anotaciones del listado:

- Líneas 191–206 (bucles de llenado): llena los buffers de envío (para los vecinos izquierdo y derecho).
- Líneas 208–217: realiza la comunicación entre los vecinos izquierdo y derecho.
- Líneas 219–234 (bucles de copia): copia los buffers de recepción dentro de las ghost cells.

Las llamadas `MPI_Irecv` y `MPI_Isend` ahora usan un conteo (count) y el tipo de datos `MPI_DOUBLE` en lugar del tipo genérico de bytes de `MPI_Pack`. También necesitamos conocer el tipo de datos para copiar datos hacia y desde el buffer de comunicación.

### 8.4.3 Intercambios de ghost cells en un cálculo de stencil tridimensional (3D)

También puede realizar un intercambio de ghost cells para un cálculo de stencil 3D. Lo haremos en el listado 8.22. Sin embargo, la configuración es un poco más complicada. Primero se calcula la disposición de los procesos como valores `xcoord`, `ycoord` y `zcoord`. Luego se determinan los vecinos y se calculan los tamaños de los datos en cada procesador.

**Listado 8.22 — Configuración para una malla 3D**

`GhostExchange/GhostExchange3D_*/GhostExchange.cc`

```cpp
63 int xcoord = rank%nprocx;
64 int ycoord = rank/nprocx%nprocy;
65 int zcoord = rank/(nprocx*nprocy);
66
67 int nleft = (xcoord > 0 ) ? rank - 1 : MPI_PROC_NULL;
68 int nrght = (xcoord < nprocx-1) ? rank + 1 : MPI_PROC_NULL;
69 int nbot = (ycoord > 0 ) ? rank - nprocx : MPI_PROC_NULL;
70 int ntop = (ycoord < nprocy-1) ? rank + nprocx : MPI_PROC_NULL;
71 int nfrnt = (zcoord > 0 ) ? rank - nprocx * nprocy : MPI_PROC_NULL;
72 int nback = (zcoord < nprocz-1) ? rank + nprocx * nprocy : MPI_PROC_NULL;
73
74 int ibegin = imax *(xcoord )/nprocx;
75 int iend = imax *(xcoord+1)/nprocx;
76 int isize = iend - ibegin;
77 int jbegin = jmax *(ycoord )/nprocy;
78 int jend = jmax *(ycoord+1)/nprocy;
79 int jsize = jend - jbegin;
80 int kbegin = kmax *(zcoord )/nprocz;
81 int kend = kmax *(zcoord+1)/nprocz;
82 int ksize = kend - kbegin;
```

Anotaciones del listado:

- Líneas 63–65 (`xcoord`, `ycoord`, `zcoord`): configura las coordenadas del proceso.
- Líneas 67–72 (`nleft` … `nback`): calcula los procesos vecinos de cada proceso.
- Líneas 74–82 (`ibegin` … `ksize`): calcula los índices de inicio y fin para cada proceso y luego el tamaño.

La actualización de ghost cells, incluyendo las copias de arreglos a los buffers, la comunicación y la copia de salida, tiene un par de cientos de líneas de longitud y no puede mostrarse aquí. Consulte los ejemplos de código (<https://github.com/EssentialsofParallelComputing/Chapter8>) que acompañan al capítulo para ver la implementación detallada. Mostraremos una versión con tipos de datos de MPI de la actualización de ghost cells en la sección 8.5.1.
