# Capítulo 8 — MPI: Guía de estudio

## 1. Fundamentos de un programa MPI

MPI es un estándar **basado en bibliotecas** (no requiere compilador especial). Todo programa empieza con `MPI_Init` y termina con `MPI_Finalize`.

### Llamadas obligatorias

```c
#include <mpi.h>
MPI_Init(&argc, &argv);                      // justo al inicio, pasando argumentos de main
MPI_Comm_rank(MPI_COMM_WORLD, &rank);        // rank: id del proceso (0 .. nprocs-1)
MPI_Comm_size(MPI_COMM_WORLD, &nprocs);      // nprocs: nº total de procesos
MPI_Finalize();                              // sincroniza y sale
```

- **Proceso**: unidad de cómputo independiente con su propia memoria.
- **Rank**: identificador único entero del proceso dentro del comunicador.
- **Comunicador**: grupo de procesos que pueden comunicarse. El por defecto es `MPI_COMM_WORLD`.
- La salida (`printf`) de distintos ranks aparece en **orden arbitrario**.

### Compilación y ejecución

- Wrappers del compilador: `mpicc` (C), `mpicxx`/`mpic++` (C++), `mpifort` (Fortran).
  - Ver flags reales: OpenMPI → `--showme`, `--showme:compile`, `--showme:link`; MPICH → `-show`, `-compile_info`, `-link_info`.
- Arranque: `mpirun -n <nprocs>` (también `mpiexec`, `aprun`, `srun`). Opción nº procesos: `-n` (a veces `-np`).

```bash
mpirun -n 4 ./mi_programa
```

---

## 2. Comunicación punto a punto (send / receive)

Un mensaje MPI = **triplete de datos** + **sobre (envelope)**:
- **Datos**: `(puntero a buffer, count, datatype)`. Tipo y count pueden diferir entre emisor y receptor (permite conversión, p. ej. endianness). El buffer de recepción **puede ser mayor**, nunca menor que el enviado.
- **Sobre**: `(rank, tag, comunicador)`. Identifica origen/destino y distingue mensajes. **Comunicador y tag deben coincidir** para completar el mensaje. `tag` puede ser `MPI_ANY_TAG`.

> Buena práctica: **publicar el receive antes** que el send, para que exista el buffer de destino.

### Prototipos básicos (blocking)

```c
MPI_Send(void *data, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
MPI_Recv(void *data, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
```

**Blocking** = no retornan hasta que el buffer es seguro de reusar (send: ya leído; recv: ya lleno).

### Riesgo de cuelgue (deadlock)

| Orden | Resultado |
|---|---|
| `Recv` antes de `Send` en ambos | **Siempre se cuelga** (recv bloquea esperando datos que nunca se envían). |
| `Send` antes de `Recv` en ambos | **Falla con mensajes grandes** (send espera buffer de recv que nunca se publica). Con mensajes pequeños MPI los copia a buffer interno y funciona por suerte. |
| Alternar por rank (pares envían, impares reciben) | Funciona, pero requiere condicionales (`if (rank%2==0)`) → propenso a errores. |

```c
// Alternancia por rank (listado 8.5)
if (rank%2 == 0) { MPI_Send(...); MPI_Recv(...); }
else             { MPI_Recv(...); MPI_Send(...); }
```

### Soluciones recomendadas

**a) `MPI_Sendrecv`** — combina send y receive; MPI evita el deadlock:

```c
MPI_Sendrecv(xsend, count, MPI_DOUBLE, partner, tag,
             xrecv, count, MPI_DOUBLE, partner, tag, comm, MPI_STATUS_IGNORE);
```

**b) No-bloqueantes / inmediatas (`I`)** — retornan de inmediato, luego se espera:

```c
MPI_Request requests[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
MPI_Irecv(xrecv, count, MPI_DOUBLE, partner, tag, comm, &requests[0]); // recv primero
MPI_Isend(xsend, count, MPI_DOUBLE, partner, tag, comm, &requests[1]);
MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);
```
- No modificar el buffer de envío ni leer el de recepción hasta que la operación complete.
- Reduce los puntos de bloqueo (un solo `Waitall` frente a múltiples bloqueos) → mejor rendimiento.

**c) Mixta** (Isend + Recv bloqueante):
```c
MPI_Isend(xsend, ..., &request);
MPI_Recv(xrecv, ..., MPI_STATUS_IGNORE);
MPI_Request_free(&request);   // evitar fuga del handle (o vía MPI_Wait/MPI_Test)
```

### Modos de send (prefijos)
`B` buffered, `S` synchronous, `R` ready, e inmediatos `IB`, `IS`, `IR`.

### Tipos de datos comunes (C)
`MPI_CHAR` (1B), `MPI_INT` (4B), `MPI_FLOAT` (4B), `MPI_DOUBLE` (8B), `MPI_BYTE` (genérico sin tipo), `MPI_PACKED` (datos empaquetados, usado con `MPI_Pack`). `MPI_BYTE` y `MPI_PACKED` coinciden con cualquier tipo.

### Comprobación de finalización
`MPI_Test` / `MPI_Testany` / `MPI_Testall` / `MPI_Testsome`, `MPI_Wait` / `MPI_Waitany` / `MPI_Waitall` / `MPI_Waitsome`, `MPI_Probe`.

---

## 3. Comunicación colectiva

Operan sobre **todos** los procesos de un comunicador. **Todos** los miembros deben llamarla o el programa se cuelga. Delegan la corrección y el rendimiento en la biblioteca.

| Operación | Qué hace |
|---|---|
| `MPI_Barrier` | Sincroniza todos los procesos (única colectiva que no mueve datos). |
| `MPI_Bcast` | Un proceso envía un mismo dato a todos. |
| `MPI_Reduce` | Combina valores de todos (suma, max…) → resultado en **un** proceso (root). |
| `MPI_Allreduce` | Como Reduce, pero el resultado queda en **todos**. |
| `MPI_Scatter`(`v`) | Reparte trozos de un arreglo del root a cada proceso. |
| `MPI_Gather`(`v`) | Reúne un valor de cada proceso, apilándolos en el root. |
| `MPI_Allgather`(`v`) | Como Gather, pero el arreglo completo queda en todos. |

Las variantes `v` (`Scatterv`/`Gatherv`/`Allgatherv`) permiten **cantidades variables** por proceso (con arreglos de counts y offsets). Las `Alltoall` existen pero son costosas y raras.

### 3.1 Barrera para sincronizar temporizadores
```c
MPI_Barrier(MPI_COMM_WORLD);
start = MPI_Wtime();      // tiempo actual
... trabajo ...
MPI_Barrier(MPI_COMM_WORLD);
elapsed = MPI_Wtime() - start;   // tras la 2ª barrera → tiempo máximo entre procesos
```
> Barreras y temporizadores sincronizados **no usar en producción**: causan ralentizaciones.

### 3.2 Broadcast para entrada de archivos pequeños
El sistema de archivos es serial y lento → abrir/leer en **un solo proceso (rank 0)** y difundir. Difundir bloques grandes, no muchos valores pequeños.
```c
if (rank == 0) { /* fopen, ftell para tamaño, fread a input_string */ }
MPI_Bcast(&input_size, 1, MPI_INT, 0, MPI_COMM_WORLD);     // primero el tamaño
if (rank != 0) input_string = malloc(input_size+1);         // los demás reservan
MPI_Bcast(input_string, input_size, MPI_CHAR, 0, MPI_COMM_WORLD); // luego los datos
```
Para escalar variable: usar `&` con la dirección. Origen = rank que posee los datos (aquí 0).

### 3.3 Reducción
Operaciones: `MPI_MAX`, `MPI_MIN`, `MPI_SUM`, `MPI_MINLOC` (índice del mín), `MPI_MAXLOC` (índice del máx).
```c
MPI_Reduce(&main_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
// promedio = SUM / nprocs (en root)
```
Resultado en el root (argumento 6). Si todos lo necesitan → `MPI_Allreduce`.

**Operador y tipo personalizados** (ej. suma de Kahan double-double):
```c
struct esum_type { double sum; double correction; };
MPI_Datatype EPSUM_TWO_DOUBLES;  MPI_Op KAHAN_SUM;

void kahan_sum(struct esum_type *in, struct esum_type *inout, int *len, MPI_Datatype *dt){
    double t = in->sum + (in->correction + inout->correction);
    double s = inout->sum + t;
    inout->correction = t - (s - inout->sum);
    inout->sum = s;
}
// init (una sola vez):
MPI_Type_contiguous(2, MPI_DOUBLE, &EPSUM_TWO_DOUBLES);
MPI_Type_commit(&EPSUM_TWO_DOUBLES);
MPI_Op_create((MPI_User_function *)kahan_sum, /*commutative=*/1, &KAHAN_SUM);
// uso:
MPI_Allreduce(&local, &global, 1, EPSUM_TWO_DOUBLES, KAHAN_SUM, MPI_COMM_WORLD);
// liberar antes de MPI_Finalize:
MPI_Type_free(&EPSUM_TWO_DOUBLES);  MPI_Op_free(&KAHAN_SUM);
```

### 3.4 Gather para ordenar impresiones de depuración
Reúne valores en el root e imprime solo desde rank 0 → salida ordenada.
```c
double times[nprocs];
MPI_Gather(&total_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
if (rank == 0) for (int i=0;i<nprocs;i++) printf("%d:Work took %lf secs\n", i, times[i]);
```
Escalar de origen necesita `&`; el arreglo de destino ya es dirección.

### 3.5 Scatter + Gather para distribuir y recuperar trabajo
```c
// 1) tamaño local por proceso (reparto equitativo con aritmética entera)
long ibegin = ncells*rank/nprocs, iend = ncells*(rank+1)/nprocs;
int nsize = iend - ibegin;
// 2) calcular nsizes[] y offsets[] de todos (necesarios para las v):
MPI_Allgather(&nsize, 1, MPI_INT, nsizes, 1, MPI_INT, comm);
offsets[0]=0; for(i=1;i<nprocs;i++) offsets[i]=offsets[i-1]+nsizes[i-1];
// 3) distribuir desde root (0):
MPI_Scatterv(a_global, nsizes, offsets, MPI_DOUBLE, a, nsize, MPI_DOUBLE, 0, comm);
... cómputo local ...
// 4) recuperar al root:
MPI_Gatherv(a, nsize, MPI_DOUBLE, a_test, nsizes, offsets, MPI_DOUBLE, 0, comm);
```
> `nsizes`/`offsets` son **enteros** → limitan el tamaño de datos manejable (no hay versión `long`).

---

## 4. Paralelismo de datos

Estrategia más común. Casos: sin comunicación (stream triad) y con comunicación de frontera (ghost cells).

### 4.1 Stream triad (ancho de banda)
Sin comunicación: cada proceso opera su trozo `c[i] = a[i] + scalar*b[i]`. Mide el ancho de banda máximo del nodo. (`c[1]=c[2]` evita que el compilador elimine el bucle.)

### 4.2 Ghost cells (celdas fantasma) — la técnica clave del paralelismo de memoria distribuida
- **Halo cells**: celdas que rodean la malla. **Domain-boundary halo**: para condiciones de frontera (reflectiva, inflow, outflow, periódica); existe en serie y paralelo.
- **Ghost cells**: copias locales de datos reales que viven en el proceso vecino; solo existen para reducir comunicaciones. Solo necesarias en paralelo.
- **Ghost cell update/exchange**: refrescar esas copias agrupando la comunicación en pocos mensajes grandes.

**Clave en C**: las filas son contiguas; las columnas tienen stride (= tamaño de fila). Enviar columnas elemento a elemento es caro → hay que agruparlas.

**Configuración 2D** (vecinos; `MPI_PROC_NULL` cuando no hay vecino):
```c
int xcoord = rank%nprocx,  ycoord = rank/nprocx;
int nleft = (xcoord>0)        ? rank-1      : MPI_PROC_NULL;
int nrght = (xcoord<nprocx-1) ? rank+1      : MPI_PROC_NULL;
int nbot  = (ycoord>0)        ? rank-nprocx : MPI_PROC_NULL;
int ntop  = (ycoord<nprocy-1) ? rank+nprocx : MPI_PROC_NULL;
// índices/tamaño locales: ibegin=imax*xcoord/nprocx, iend=imax*(xcoord+1)/nprocx, isize=iend-ibegin (idem j)
```
- Memoria reservada = tamaño local + halos. Indexación desplazada (`-nhalo` .. `isize+nhalo`); celdas reales siempre `0..isize-1`.
- Bucle típico: cómputo de stencil → `SWAP_PTR(xnew,x)` → `boundarycondition_update` → `ghostcell_update`.
- Si **no** hay esquinas: intercambios horizontal y vertical simultáneos. Si **sí** hay esquinas: requiere sincronización entre horizontal y vertical.

**Carga de buffers** — dos enfoques equivalentes:
1. **`MPI_Pack`/`MPI_Unpack`** (mejor con múltiples tipos): empaqueta en buffer agnóstico al tipo (`MPI_PACKED`).
   ```c
   MPI_Pack(&x[j][0], nhalo, MPI_DOUBLE, xbuf_left_send, bufsize, &position, comm);
   // ... Irecv/Isend con MPI_PACKED ... Waitall ...
   MPI_Unpack(xbuf_rght_recv, bufsize, &position, &x[j][isize], nhalo, MPI_DOUBLE, comm);
   ```
2. **Asignación de arreglo** (mejor con un único tipo simple): copiar a mano con bucles y comunicar con `MPI_DOUBLE`.

Las filas (vertical) se comunican como datos contiguos. Con esquinas, un único buffer contiguo; sin esquinas, filas de halo individuales.

### 4.3 Ghost cells 3D
Añade `zcoord = rank/(nprocx*nprocy)` y vecinos `nfrnt = rank - nprocx*nprocy`, `nback = rank + nprocx*nprocy`. Misma idea, más direcciones (la implementación completa son cientos de líneas).

---

## 5. Funcionalidad avanzada

### 5.1 Tipos de datos personalizados de MPI
Encapsulan datos complejos → una sola llamada envía muchos fragmentos; evitan copias extra y simplifican el código (menos bugs).

| Función | Crea |
|---|---|
| `MPI_Type_contiguous` | Bloque contiguo. |
| `MPI_Type_vector` | Bloques con stride: `(count, blocklength, stride, oldtype, &newtype)`. |
| `MPI_Type_create_subarray` | Subconjunto rectangular de un arreglo mayor. |
| `MPI_Type_indexed` / `_create_hindexed` | Índices irregulares (hindexed: offsets en bytes). |
| `MPI_Type_create_struct` | Estructura portable (respeta padding del compilador). |

Ciclo de vida: **`MPI_Type_commit`** antes de usar, **`MPI_Type_free`** al final (antes de `MPI_Finalize`).

**Ejemplo 2D** (columna con stride = `horiz_type`; filas = `vert_type`):
```c
MPI_Type_vector(jnum, nhalo, isize+2*nhalo, MPI_DOUBLE, &horiz_type);
MPI_Type_commit(&horiz_type);
if (!corners) MPI_Type_vector(nhalo, isize, isize+2*nhalo, MPI_DOUBLE, &vert_type);
else          MPI_Type_contiguous(nhalo*(isize+2*nhalo), MPI_DOUBLE, &vert_type);
MPI_Type_commit(&vert_type);
// comunicación: Irecv/Isend con count=1 y el tipo personalizado, p.ej.
MPI_Irecv(&x[jlow][isize], 1, horiz_type, nrght, 1001, comm, &request[0]);
```
**Ejemplo 3D**: `MPI_Type_create_subarray(ndims, array_sizes, subarray_sizes, subarray_starts, MPI_ORDER_C, MPI_DOUBLE, &tipo)` para `horiz_type`, `vert_type`, `depth_type`.

> Razón principal de usar tipos de datos: **código más limpio y seguro** (además de posible mejora de rendimiento por evitar copias).

### 5.2 Topología cartesiana
Simplifica el cálculo de coordenadas y vecinos.
```c
int dims[2] = {nprocy, nprocx};   int periodic[2] = {0,0};   int coords[2];
MPI_Dims_create(nprocs, 2, dims);          // rellena dims=0 con valores válidos
MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periodic, /*reorder=*/0, &cart_comm);
MPI_Cart_coords(cart_comm, rank, 2, coords);
MPI_Cart_shift(cart_comm, 1, 1, &nleft, &nrght);  // dim 1, desplazamiento 1
MPI_Cart_shift(cart_comm, 0, 1, &nbot,  &ntop);
```
- `periodic`: indica fronteras que se envuelven. `reorder`: permite a MPI reordenar ranks.
- `MPI_Dims_create` ignora el tamaño de la malla (puede dar mala forma para dominios estrechos).
- 3D: `dims[3]={nprocz,nprocy,nprocx}`, tres `MPI_Cart_shift` (dims 2,1,0).

**Comunicación con vecinos en una sola llamada**:
```c
int MPI_Neighbor_alltoallw(sendbuf, sendcounts[], sdispls[], sendtypes[],
                           recvbuf, recvcounts[], rdispls[], recvtypes[], comm);
```
- Se puede operar **in place** (sendbuf = recvbuf = arreglo `x`).
- `counts` = 1 por lado (4 lados en 2D, 6 en 3D); el bloque se describe con el datatype.
- Orden de lados: 2D = inferior, superior, izquierda, derecha; 3D = frente, atrás, inferior, superior, izquierda, derecha.
- `sdispls`/`rdispls` en **bytes** (offsets × 8 para `double`).
- **Con esquinas**: dividir en llamadas por dirección con `counts` parciales (p.ej. `{0,0,1,1}` y `{1,1,0,0}`) y sincronizar entre ellas; **sin esquinas**: una sola llamada con `counts={1,1,1,1}`.

### 5.3 Rendimiento de las variantes
Tipos de datos de MPI y topología cartesiana ≈ ligeramente más rápidos (evitan una copia) y, sobre todo, **simplifican mucho el código**. Mayor coste de configuración, pero solo una vez al inicio. (En 2D, el buffer llenado a mano puede superar a los tipos; las rutinas pack son las más lentas.)

Opciones del programa GhostExchange: `-x/-y/-z` procesos por dirección, `-i/-j/-k` tamaño de malla, `-h` ancho de halo (1 o 2), `-c` incluir esquinas, `-t` timing.

---

## 6. MPI híbrido + OpenMP

Combinar MPI con OpenMP (= reemplazar algunos ranks por hilos). Útil solo en **escala extrema**; añade complejidad.

**Ventajas**: menos ghost cells entre nodos, menos memoria para buffers MPI, menos contención de la NIC, árboles de comunicación más cortos (log₂n), mejor balanceo de carga, acceso a hardware solo accesible por hilos.

**Inicialización** — reemplazar `MPI_Init` por `MPI_Init_thread`:
```c
int provided;
MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
if (provided != MPI_THREAD_FUNNELED) { /* abortar */ }
```
Modelos de hilos (de menor a mayor seguridad / coste):
- `MPI_THREAD_SINGLE` — un solo hilo.
- `MPI_THREAD_FUNNELED` — multihilo, pero solo el hilo principal llama a MPI. *(El típico: comunicación en el bucle principal, OpenMP en los bucles de cómputo.)*
- `MPI_THREAD_SERIALIZED` — multihilo, solo un hilo a la vez llama a MPI.
- `MPI_THREAD_MULTIPLE` — varios hilos llaman a MPI simultáneamente.
> Usar el nivel **más bajo** que se necesite; cada nivel superior penaliza (mutexes internos).

**Cómputo** — basta añadir pragmas:
```c
#pragma omp parallel for
for (int j=0; j<jsize; j++){
  #pragma omp simd
  for (int i=0; i<isize; i++)
    xnew[j][i] = (x[j][i]+x[j][i-1]+x[j][i+1]+x[j-1][i]+x[j+1][i])/5.0;
}
```

**Afinidad (placement)** — preferencia de planificación de proceso/hilo a un componente de hardware (también *pinning*/*binding*). Crucial para rendimiento:
```bash
export OMP_NUM_THREADS=22
export OMP_PLACES=cores
export OMP_CPU_BIND=true
mpirun -n 4 --bind-to socket ./CartExchange -x 2 -y 2 -i 20000 -j 20000 -h 2 -t -c
# --bind-to: socket | core | hwthread
```

---

## 7. Exploraciones adicionales
- **Grupos de comunicadores**: dividir/manipular `MPI_COMM_WORLD` en subgrupos (filas, columnas, tareas).
- **Mallas no estructuradas**: intercambio de frontera con bibliotecas sparse/grafos (p.ej. L7 en CLAMR).
- **Memoria compartida**: optimización interna + "ventanas" (windows) de memoria compartida.
- **Comunicación unilateral**: `MPI_Put` / `MPI_Get` (solo un lado participa activamente).

---

## Resumen
- Usa los send/receive **apropiados** (no-bloqueantes o `Sendrecv`) para evitar cuelgues y obtener rendimiento.
- Usa **comunicación colectiva** para operaciones comunes: conciso, sin cuelgues, más rápido.
- Usa **ghost exchanges** para enlazar subdominios como una única malla global.
- Añade niveles de paralelismo combinando MPI + OpenMP + vectorización para escala extrema.
