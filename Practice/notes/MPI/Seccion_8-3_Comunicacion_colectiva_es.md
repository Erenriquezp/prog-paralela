## 8.3 Comunicación colectiva: un componente poderoso de MPI

En esta sección examinaremos el rico conjunto de llamadas de comunicación colectiva de MPI. Las comunicaciones colectivas operan sobre un grupo de procesos contenido en un comunicador de MPI. Para operar sobre un conjunto parcial de procesos, puede crear su propio comunicador de MPI para un subconjunto de `MPI_COMM_WORLD`, como por ejemplo uno de cada dos procesos. Luego puede usar su comunicador en lugar de `MPI_COMM_WORLD` en las llamadas de comunicación colectiva. La mayoría de las rutinas de comunicación colectiva operan sobre datos. La figura 8.4 ofrece una idea visual de lo que hace cada operación colectiva.

> **Figura 8.4** El movimiento de datos de las rutinas colectivas de MPI más comunes proporciona funciones importantes para los programas paralelos. Las variantes adicionales `MPI_Scatterv`, `MPI_Gatherv` y `MPI_Allgatherv` permiten enviar o recibir una cantidad variable de datos desde los procesos. No se muestran algunas rutinas adicionales como `MPI_Alltoall` y funciones similares.

Contenido de la figura 8.4 (movimiento de datos de cada operación, con cuatro procesos: Proc 0–Proc 3):

- **`MPI_Bcast`** (superior izquierda): Proc 0 contiene el valor `1.0`; tras el broadcast, ese mismo `1.0` queda copiado en Proc 0, Proc 1, Proc 2 y Proc 3.
- **`MPI_Reduce`** (superior central): cada proceso aporta `1.0`; se combinan con la operación de suma (`+`) y el resultado `4.0` queda únicamente en Proc 0.
- **`MPI_Allreduce`** (superior derecha): cada proceso aporta `1.0`; se combinan con la suma (`+`) y el resultado `4.0` queda en todos los procesos (Proc 0–Proc 3).
- **`MPI_Scatter`** (inferior izquierda): Proc 0 contiene el arreglo `[1.0 2.0 3.0 4.0]`; se reparte un elemento a cada proceso: `1.0`→Proc 0, `2.0`→Proc 1, `3.0`→Proc 2, `4.0`→Proc 3.
- **`MPI_Gather`** (inferior central): cada proceso aporta un valor (`1.0`, `2.0`, `3.0`, `4.0`); se reúnen y apilan en Proc 0 como el arreglo `[1.0 2.0 3.0 4.0]`.
- **`MPI_Allgather`** (inferior derecha): cada proceso aporta un valor; se reúnen y el arreglo completo `[1.0 2.0 3.0 4.0]` queda en cada uno de los procesos (Proc 0–Proc 3).

Presentaremos ejemplos de cómo usar las operaciones colectivas más comúnmente utilizadas, tal como podrían aplicarse en una aplicación. El primer ejemplo (en la sección 8.3.1) muestra cómo podría usar la barrera (barrier). Es la única rutina colectiva que no opera sobre datos. Luego mostraremos algunos ejemplos con el broadcast (sección 8.3.2), la reducción (sección 8.3.3) y, por último, las operaciones de scatter/gather (secciones 8.3.4 y 8.3.5). MPI también tiene una variedad de rutinas all-to-all (todos a todos). Pero estas son costosas y rara vez se usan, así que no las cubriremos aquí. Todas estas operaciones colectivas operan sobre un grupo de procesos representado por un grupo de comunicación. Todos los miembros de un grupo de comunicación deben llamar a la colectiva, o de lo contrario su programa se colgará.

### 8.3.1 Uso de una barrera para sincronizar temporizadores

La llamada de comunicación colectiva más simple es `MPI_Barrier`. Se usa para sincronizar todos los procesos de un comunicador de MPI. En la mayoría de los programas no debería ser necesaria, pero a menudo se usa para depuración (debugging) y para sincronizar temporizadores (timers). Veamos cómo se podría usar `MPI_Barrier` para sincronizar temporizadores en el siguiente listado. También usamos la función `MPI_Wtime` para obtener la hora actual.

**Listado 8.9 — Uso de MPI_Barrier para sincronizar un temporizador en un programa MPI**

`SynchronizedTimer/SynchronizedTimer1.c`

```c
 1 #include <mpi.h>
 2 #include <unistd.h>
 3 #include <stdio.h>
 4 int main(int argc, char *argv[])
 5 {
 6 double start_time, main_time;
 7
 8 MPI_Init(&argc, &argv);
 9 int rank;
10 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
11
12 MPI_Barrier(MPI_COMM_WORLD);
13 start_time = MPI_Wtime();
14
15 sleep(30);
16
17 MPI_Barrier(MPI_COMM_WORLD);
18 main_time = MPI_Wtime() - start_time;
19 if (rank == 0) printf("Time for work is %lf seconds\n", main_time);
20
21 MPI_Finalize();
22 return 0;
23 }
```

Anotaciones del listado:

- Línea 12 (`MPI_Barrier`): sincroniza todos los procesos para que comiencen aproximadamente al mismo tiempo.
- Línea 13 (`start_time = MPI_Wtime()`): obtiene el valor inicial del temporizador usando la rutina `MPI_Wtime`.
- Línea 15 (`sleep(30)`): representa el trabajo que se está realizando.
- Línea 17 (`MPI_Barrier`): sincroniza los procesos para obtener el tiempo más largo empleado.
- Línea 18 (`main_time = ...`): obtiene el valor del temporizador y le resta el valor inicial para obtener el tiempo transcurrido.

La barrera se inserta antes de iniciar el temporizador y luego justo antes de detenerlo. Esto fuerza a que los temporizadores de todos los procesos comiencen aproximadamente al mismo tiempo. Al insertar la barrera antes de detener el temporizador, obtenemos el tiempo máximo entre todos los procesos. A veces, usar un temporizador sincronizado proporciona una medida del tiempo menos confusa, pero en otros casos un temporizador no sincronizado es mejor.

> **NOTA** Los temporizadores sincronizados y las barreras no deberían usarse en ejecuciones de producción; pueden causar ralentizaciones graves en una aplicación.

### 8.3.2 Uso del broadcast para manejar la entrada de archivos pequeños

El broadcast envía datos desde un procesador a todos los demás. Esta operación se muestra en la figura 8.4, en la parte superior izquierda. Uno de los usos del broadcast, `MPI_Bcast`, es enviar valores leídos de un archivo de entrada a todos los demás procesos. Si cada proceso intenta abrir un archivo con un número grande de procesos, la apertura del archivo puede tardar minutos en completarse. Esto se debe a que los sistemas de archivos son inherentemente seriales y uno de los componentes más lentos de un sistema computacional. Por estas razones, para la entrada de archivos pequeños, es una buena práctica abrir y leer un archivo desde un único proceso. El siguiente listado muestra la manera de hacerlo.

**Listado 8.10 — Uso de MPI_Bcast para manejar la entrada de archivos pequeños**

`FileRead/FileRead.c`

```c
 1 #include <stdio.h>
 2 #include <string.h>
 3 #include <stdlib.h>
 4 #include <mpi.h>
 5 int main(int argc, char *argv[])
 6 {
 7 int rank, input_size;
 8 char *input_string, *line;
 9 FILE *fin;
10
11 MPI_Init(&argc, &argv);
12 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
13
14 if (rank == 0){
15 fin = fopen("file.in", "r");
16 fseek(fin, 0, SEEK_END);
17 input_size = ftell(fin);
18 fseek(fin, 0, SEEK_SET);
19 input_string = (char *)malloc((input_size+1)*sizeof(char));
20 fread(input_string, 1, input_size, fin);
21 input_string[input_size] = '\0';
22 }
23
24 MPI_Bcast(&input_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
25 if (rank != 0) input_string = (char *)malloc((input_size+1)*sizeof(char));
26 MPI_Bcast(input_string, input_size, MPI_CHAR, 0, MPI_COMM_WORLD);
27
28 if (rank == 0) fclose(fin);
29
30 line = strtok(input_string,"\n");
31 while (line != NULL){
32 printf("%d:input string is %s\n",rank,line);
33 line = strtok(NULL,"\n");
34 }
35 free(input_string);
36
37 MPI_Finalize();
38 return 0;
39 }
```

Anotaciones del listado:

- Líneas 16–17 (`fseek` con `SEEK_END`, `ftell`): obtiene el tamaño del archivo para asignar un buffer de entrada.
- Línea 18 (`fseek` con `SEEK_SET`): reposiciona el puntero del archivo al inicio del archivo.
- Línea 20 (`fread`): lee el archivo completo.
- Línea 21 (`input_string[input_size] = '\0'`): termina con null el buffer de entrada.
- Línea 24 (`MPI_Bcast`): hace broadcast del tamaño del buffer de entrada.
- Línea 25 (`malloc`): asigna el buffer de entrada en los demás procesos.
- Línea 26 (`MPI_Bcast`): hace broadcast del buffer de entrada.

Es mejor hacer broadcast de bloques de datos más grandes que hacer broadcast de muchos valores individuales pequeños. Por lo tanto, hacemos broadcast del archivo completo. Para ello, primero necesitamos hacer broadcast del tamaño, de modo que cada proceso pueda asignar un buffer de entrada, y luego hacer broadcast de los datos. La lectura del archivo y los broadcasts se realizan desde el rank 0, generalmente denominado el proceso principal (main process).

`MPI_Bcast` toma un puntero como primer argumento, así que al enviar una variable escalar enviamos la referencia usando el operador ampersand (`&`) para obtener la dirección de la variable. Luego vienen el conteo (count) y el tipo (type) para definir completamente los datos que se van a enviar. El siguiente argumento especifica el proceso de origen. Es 0 en ambas llamadas porque ese es el rank donde residen los datos. Todos los demás procesos del comunicador `MPI_COMM_WORLD` reciben entonces los datos. Esta técnica es para archivos de entrada pequeños. Para la entrada o salida de archivos más grandes, hay formas de realizar operaciones de archivo en paralelo. El complejo mundo de la entrada y salida en paralelo se discute en el capítulo 16.

### 8.3.3 Uso de una reducción para obtener un único valor de todos los procesos

El patrón de reducción, discutido en la sección 5.7, es uno de los patrones de cómputo paralelo más importantes. La operación de reducción se muestra en la figura 8.4, en la parte superior central. Un ejemplo de la reducción en la sintaxis de arreglos de Fortran es `xsum = sum(x(:))`, donde la función intrínseca `sum` de Fortran suma el arreglo `x` y lo coloca en la variable escalar `xsum`. Las llamadas de reducción de MPI toman un arreglo o un arreglo multidimensional y combinan los valores en un resultado escalar. Hay muchas operaciones que pueden realizarse durante la reducción. Las más comunes son:

- `MPI_MAX` (valor máximo de un arreglo)
- `MPI_MIN` (valor mínimo de un arreglo)
- `MPI_SUM` (suma de un arreglo)
- `MPI_MINLOC` (índice del valor mínimo)
- `MPI_MAXLOC` (índice del valor máximo)

El siguiente listado muestra cómo podemos usar `MPI_Reduce` para obtener el mínimo, el máximo y el promedio de una variable de cada proceso.

**Listado 8.11 — Uso de reducciones para obtener los resultados mínimo, máximo y promedio del temporizador**

`SynchronizedTimer/SynchronizedTimer2.c`

```c
 1 #include <mpi.h>
 2 #include <unistd.h>
 3 #include <stdio.h>
 4 int main(int argc, char *argv[])
 5 {
 6 double start_time, main_time, min_time, max_time, avg_time;
 7
 8 MPI_Init(&argc, &argv);
 9 int rank, nprocs;
10 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
11 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
12
13 MPI_Barrier(MPI_COMM_WORLD);
14 start_time = MPI_Wtime();
15
16 sleep(30);
17
18 main_time = MPI_Wtime() - start_time;
19 MPI_Reduce(&main_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
20 MPI_Reduce(&main_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0,MPI_COMM_WORLD);
21 MPI_Reduce(&main_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0,MPI_COMM_WORLD);
22 if (rank == 0) printf("Time for work is Min: %lf Max: %lf Avg: %lf seconds\n",
23                       min_time, max_time, avg_time/nprocs);
24
25 MPI_Finalize();
26 return 0;
27 }
```

Anotaciones del listado:

- Línea 13 (`MPI_Barrier`): sincroniza todos los procesos para que comiencen aproximadamente al mismo tiempo.
- Línea 16 (`sleep(30)`): representa el trabajo que se está realizando.
- Línea 18 (`main_time = ...`): obtiene el valor del temporizador y le resta el valor inicial para obtener el tiempo transcurrido.
- Líneas 19–21 (`MPI_Reduce`): usa llamadas de reducción para calcular el tiempo máximo, mínimo y promedio.

El resultado de la reducción, el máximo en este caso, se almacena en el rank 0 (argumento 6 en la llamada `MPI_Reduce`), que en este caso es el proceso principal. Si solo quisiéramos imprimirlo en el proceso principal, esto sería apropiado. Pero si quisiéramos que todos los procesos tuvieran el valor, usaríamos la rutina `MPI_Allreduce`.

También puede definir su propio operador. Usaremos el ejemplo de la suma de precisión mejorada de Kahan (Kahan enhanced-precision summation) con la que hemos estado trabajando y que introdujimos por primera vez en la sección 5.7. El desafío en un entorno paralelo de memoria distribuida es trasladar la suma de Kahan a través de los ranks de los procesos. Comenzamos observando el programa principal en el siguiente listado, antes de ver otras dos partes del programa en los listados 8.13 y 8.14.

**Listado 8.12 — Una versión MPI de la suma de Kahan**

`GlobalSums/globalsums.c`

```c
57 int main(int argc, char *argv[])
58 {
59 MPI_Init(&argc, &argv);
60 int rank, nprocs;
61 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
62 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
63
64 init_kahan_sum();
65
66 if (rank == 0) printf("MPI Kahan tests\n");
67
68 for (int pow_of_two = 8; pow_of_two < 31; pow_of_two++){
69 long ncells = (long)pow((double)2,(double)pow_of_two);
70
71 int nsize;
72 double accurate_sum;
73 double *local_energy = init_energy(ncells, &nsize, &accurate_sum);
74
75 struct timespec cpu_timer;
76 cpu_timer_start(&cpu_timer);
77
78 double test_sum = global_kahan_sum(nsize, local_energy);
79
80 double cpu_time = cpu_timer_stop(cpu_timer);
81
82 if (rank == 0){
83 double sum_diff = test_sum-accurate_sum;
84 printf("ncells %ld log %d acc sum %-17.16lg sum %-17.16lg ",
85       ncells,(int)log2((double)ncells),accurate_sum,test_sum);
86 printf("diff %10.4lg relative diff %10.4lg runtime %lf\n",
87       sum_diff,sum_diff/accurate_sum, cpu_time);
88 }
89
90 free(local_energy);
91 }
92
93 MPI_Type_free(&EPSUM_TWO_DOUBLES);
94 MPI_Op_free(&KAHAN_SUM);
95 MPI_Finalize();
96 return 0;
97 }
```

Anotaciones del listado:

- Línea 64 (`init_kahan_sum`): inicializa el nuevo tipo de datos de MPI y crea un nuevo operador.
- Línea 73 (`init_energy`): obtiene un arreglo distribuido con el que trabajar.
- Línea 78 (`global_kahan_sum`): calcula la suma de Kahan del arreglo de energía a través de todos los procesos.
- Líneas 93–94 (`MPI_Type_free`, `MPI_Op_free`): libera el tipo de datos y el operador personalizados.

El programa principal muestra que el nuevo tipo de datos de MPI se crea una vez al inicio del programa y se libera al final, antes de `MPI_Finalize`. La llamada para realizar la suma global de Kahan se realiza varias veces dentro del bucle, donde el tamaño de los datos se incrementa en potencias de dos. Ahora veamos el siguiente listado para ver qué hay que hacer para inicializar el nuevo tipo de datos y el operador.

**Listado 8.13 — Inicialización del nuevo tipo de datos y operador de MPI para la suma de Kahan**

`GlobalSums/globalsums.c`

```c
14 struct esum_type{
15 double sum;
16 double correction;
17 };
18
19 MPI_Datatype EPSUM_TWO_DOUBLES;
20 MPI_Op KAHAN_SUM;
21
22 void kahan_sum(struct esum_type * in, struct esum_type * inout, int *len,
23 MPI_Datatype *EPSUM_TWO_DOUBLES)
24 {
25 double corrected_next_term, new_sum;
26 corrected_next_term = in->sum + (in->correction + inout->correction);
27 new_sum = inout->sum + corrected_next_term;
28 inout->correction = corrected_next_term - (new_sum - inout->sum);
29 inout->sum = new_sum;
30 }
31
32 void init_kahan_sum(void){
33 MPI_Type_contiguous(2, MPI_DOUBLE, &EPSUM_TWO_DOUBLES);
34 MPI_Type_commit(&EPSUM_TWO_DOUBLES);
35
36 int commutative = 1;
37 MPI_Op_create((MPI_User_function *)kahan_sum, commutative, &KAHAN_SUM);
38 }
```

Anotaciones del listado:

- Líneas 14–17 (`struct esum_type`): define una estructura `esum_type` para contener la suma y el término de corrección.
- Línea 19 (`MPI_Datatype EPSUM_TWO_DOUBLES`): declara un nuevo tipo de datos de MPI compuesto por dos `double`.
- Línea 20 (`MPI_Op KAHAN_SUM`): declara un nuevo operador de suma de Kahan.
- Líneas 22–23 (`void kahan_sum(...)`): define una función para el nuevo operador usando una firma (signature) predefinida.
- Líneas 33–34 (`MPI_Type_contiguous`, `MPI_Type_commit`): crea el tipo y lo confirma (commit).
- Líneas 36–37 (`MPI_Op_create`): crea el nuevo operador y lo confirma (commit).

Primero creamos el nuevo tipo de datos, `EPSUM_TWO_DOUBLES`, combinando dos del tipo de datos básico `MPI_DOUBLE` en la línea 33. Tenemos que declarar el tipo fuera de la rutina, en la línea 19, para que esté disponible para su uso por la rutina de suma. Para crear el nuevo operador, primero escribimos la función que se usará como operador en las líneas 22-30. Luego usamos `esum_type` para pasar ambos valores `double` de entrada y de vuelta a la salida. También necesitamos pasar la longitud y el tipo de datos sobre el que operará, como el nuevo tipo `EPSUM_TWO_DOUBLES`.

En el proceso de crear un operador de reducción de suma de Kahan, le mostramos cómo crear un nuevo tipo de datos de MPI y un nuevo operador de reducción de MPI. Ahora pasemos a calcular efectivamente la suma global del arreglo a través de los ranks de MPI, como muestra el siguiente listado.

**Listado 8.14 — Realización de una suma de Kahan en MPI**

`GlobalSums/globalsums.c`

```c
40 double global_kahan_sum(int nsize, double *local_energy){
41 struct esum_type local, global;
42 local.sum = 0.0;
43 local.correction = 0.0;
44
45 for (long i = 0; i < nsize; i++) {
46 double corrected_next_term = local_energy[i] + local.correction;
47 double new_sum = local.sum + local.correction;
48 local.correction = corrected_next_term - (new_sum - local.sum);
49 local.sum = new_sum;
50 }
51
52 MPI_Allreduce(&local, &global, 1, EPSUM_TWO_DOUBLES, KAHAN_SUM, MPI_COMM_WORLD);
53
54 return global.sum;
55 }
```

Anotaciones del listado:

- Líneas 42–43 (`local.sum`, `local.correction`): inicializa a cero ambos miembros de la `esum_type`.
- Líneas 45–50 (bucle `for`): realiza la suma de Kahan dentro del proceso (on-process).
- Línea 52 (`MPI_Allreduce`): realiza la reducción con el nuevo operador `KAHAN_SUM`.

Calcular la suma global de Kahan es relativamente fácil ahora. Podemos hacer la suma local de Kahan como se mostró en la sección 5.7. Pero tenemos que añadir `MPI_Allreduce` en la línea 52 para obtener el resultado global. Aquí definimos la operación allreduce para que termine con el resultado en todos los procesadores, como se muestra en la figura 8.4 en la parte superior derecha.

### 8.3.4 Uso de gather para ordenar las impresiones de depuración

Una operación gather puede describirse como una operación de colación (collate), donde los datos de todos los procesadores se reúnen y se apilan en un único arreglo, como se muestra en la figura 8.4 en la parte inferior central. Puede usar esta llamada de comunicación colectiva para poner orden en la salida hacia la consola de su programa. A estas alturas, debería haber notado que la salida impresa desde múltiples ranks de un programa MPI sale en orden aleatorio, produciendo un desorden confuso y revuelto. Veamos una mejor manera de manejar esto, de modo que la única salida provenga del proceso principal. Al imprimir la salida solo desde el proceso principal, el orden será correcto. El siguiente listado muestra un programa de ejemplo que obtiene datos de todos los procesos y los imprime en una salida ordenada y agradable.

**Listado 8.15 — Uso de un gather para imprimir mensajes de depuración**

`DebugPrintout/DebugPrintout.c`

```c
 1 #include <stdio.h>
 2 #include <time.h>
 3 #include <unistd.h>
 4 #include <mpi.h>
 5 #include "timer.h"
 6 int main(int argc, char *argv[])
 7 {
 8 int rank, nprocs;
 9 double total_time;
10 struct timespec tstart_time;
11
12 MPI_Init(&argc, &argv);
13 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
14 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
15
16 cpu_timer_start(&tstart_time);
17 sleep(30);
18 total_time += cpu_timer_stop(tstart_time);
19
20 double times[nprocs];
21 MPI_Gather(&total_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
22 if (rank == 0) {
23 for (int i=0; i<nprocs; i++){
24 printf("%d:Work took %lf secs\n", i, times[i]);
25 }
26 }
27
28 MPI_Finalize();
29 return 0;
30 }
```

Anotaciones del listado:

- Líneas 16–18 (bloque de temporización): obtiene valores únicos en cada proceso para nuestro ejemplo.
- Línea 20 (`double times[nprocs]`): necesita un arreglo para recopilar todos los tiempos.
- Línea 21 (`MPI_Gather`): usa un gather para llevar todos los valores al proceso cero.
- Línea 22 (`if (rank == 0)`): solo imprime en el proceso principal.
- Línea 23 (bucle `for`): itera sobre los procesos para la impresión.
- Línea 24 (`printf`): imprime el tiempo de cada proceso.

`MPI_Gather` toma el triplete estándar que describe la fuente de datos. Necesitamos usar el ampersand para obtener la dirección de la variable escalar `total_time`. El destino también es un triplete con el arreglo de destino `times`. Un arreglo ya es una dirección, así que no se necesita ampersand. El gather se realiza hacia el proceso 0 del grupo de comunicación del mundo de MPI (MPI world). Desde ahí, se requiere un bucle para imprimir el tiempo de cada proceso. Anteponemos a cada línea un número con el formato `#:` para que quede claro a qué proceso se refiere la salida.

### 8.3.5 Uso de scatter y gather para enviar datos a los procesos para su procesamiento

La operación scatter, mostrada en la figura 8.4 en la parte inferior izquierda, es lo opuesto a la operación gather. En esta operación, los datos se envían desde un proceso a todos los demás del grupo de comunicación. El uso más común de una operación de scattering es en la estrategia paralela de distribuir arreglos de datos a otros procesos para trabajar. Esto lo proporcionan las rutinas `MPI_Scatter` y `MPI_Scatterv`. El siguiente listado muestra la implementación.

**Listado 8.16 — Uso de scatter para distribuir datos y gather para recuperarlos**

`ScatterGather/ScatterGather.c`

```c
 1 #include <stdio.h>
 2 #include <stdlib.h>
 3 #include <mpi.h>
 4 int main(int argc, char *argv[])
 5 {
 6 int rank, nprocs, ncells = 100000;
 7
 8 MPI_Init(&argc, &argv);
 9 MPI_Comm comm = MPI_COMM_WORLD;
10 MPI_Comm_rank(comm, &rank);
11 MPI_Comm_size(comm, &nprocs);
12
13 long ibegin = ncells *(rank )/nprocs;
14 long iend = ncells *(rank+1)/nprocs;
15 int nsize = (int)(iend-ibegin);
16
17 double *a_global, *a_test;
18 if (rank == 0) {
19 a_global = (double *)malloc(ncells*sizeof(double));
20 for (int i=0; i<ncells; i++) {
21 a_global[i] = (double)i;
22 }
23 }
24
25 int nsizes[nprocs], offsets[nprocs];
26 MPI_Allgather(&nsize, 1, MPI_INT, nsizes, 1, MPI_INT, comm);
27 offsets[0] = 0;
28 for (int i = 1; i<nprocs; i++){
29 offsets[i] = offsets[i-1] + nsizes[i-1];
30 }
31
32 double *a = (double *)malloc(nsize*sizeof(double));
33 MPI_Scatterv(a_global, nsizes, offsets,
34              MPI_DOUBLE, a, nsize, MPI_DOUBLE, 0, comm);
35
36 for (int i=0; i<nsize; i++){
37 a[i] += 1.0;
38 }
39
40 if (rank == 0) {
41 a_test = (double *)malloc(ncells*sizeof(double));
42 }
43
44 MPI_Gatherv(a, nsize, MPI_DOUBLE,
45             a_test, nsizes, offsets, MPI_DOUBLE, 0, comm);
46
47 if (rank == 0){
48 int ierror = 0;
49 for (int i=0; i<ncells; i++){
50 if (a_test[i] != a_global[i] + 1.0) {
51 printf("Error: index %d a_test %lf a_global %lf\n",
52       i,a_test[i],a_global[i]);
53 ierror++;
54 }
55 }
56 printf("Report: Correct results %d errors %d\n", ncells-ierror,ierror);
57 }
58
59 free(a);
60 if (rank == 0) {
61 free(a_global);
62 free(a_test);
63 }
64
65 MPI_Finalize();
66 return 0;
67 }
```

Anotaciones del listado:

- Líneas 13–15: calcula el tamaño del arreglo en cada proceso.
- Líneas 18–23: configura los datos en el proceso principal.
- Líneas 25–30 (`nsizes`/`offsets` + `MPI_Allgather`): obtiene los tamaños y desplazamientos (offsets) en los arreglos globales para la comunicación.
- Líneas 32–34 (`malloc` + `MPI_Scatterv`): distribuye los datos a los demás procesos.
- Líneas 36–38 (bucle `for`): realiza el cómputo.
- Líneas 44–45 (`MPI_Gatherv`): devuelve los datos del arreglo al proceso principal, quizás para su salida.

Primero necesitamos calcular el tamaño de los datos en cada proceso. La distribución deseada debe ser lo más equitativa posible. Una forma simple de calcular el tamaño se muestra en las líneas 13-15, usando aritmética entera simple. Ahora necesitamos el arreglo global, pero solo lo necesitamos en el proceso principal. Así que lo asignamos y lo configuramos en este proceso en las líneas 18-23. Para distribuir o reunir (gather) los datos, deben conocerse los tamaños y los desplazamientos (offsets) de todos los procesos. Vemos el cálculo típico para esto en las líneas 25-30. El scatter propiamente dicho se realiza con un `MPI_Scatterv` en las líneas 32-34. La fuente de datos se describe con los argumentos buffer, counts, offsets y el tipo de datos. El destino se maneja con el triplete estándar. Luego se especifica el rank de origen que enviará los datos, que es el rank 0. Finalmente, el último argumento es `comm`, el grupo de comunicación que recibirá los datos.

`MPI_Gatherv` realiza la operación opuesta, como se muestra en la figura 8.4. Solo necesitamos el arreglo global en el proceso principal, por lo que solo se asigna allí, en las líneas 40-42. Los argumentos de `MPI_Gatherv` comienzan con la descripción de la fuente mediante el triplete estándar. Luego, el destino se describe con los mismos cuatro argumentos que se usaron en el scatter. El rank de destino es el siguiente argumento, seguido del grupo de comunicación.

Cabe señalar que los tamaños y desplazamientos (offsets) usados en la llamada `MPI_Gatherv` son todos de tipo entero. Esto limita el tamaño de los datos que pueden manejarse. Hubo un intento de cambiar el tipo de datos al tipo `long`, para que se pudieran manejar tamaños de datos más grandes, en la versión 3 del estándar MPI. No fue aprobado porque rompería demasiadas aplicaciones. Manténgase atento a la incorporación de nuevas llamadas que proporcionen soporte para un tipo entero `long` en uno de los próximos estándares de MPI.
