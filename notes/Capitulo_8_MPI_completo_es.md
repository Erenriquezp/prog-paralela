# Capítulo 8. MPI: la columna vertebral del paralelismo

**Este capítulo cubre:**

- El envío de mensajes de un proceso a otro
- La ejecución de patrones de comunicación comunes mediante llamadas colectivas de MPI
- El enlace de mallas situadas en procesos separados mediante intercambios de comunicación
- La creación de tipos de datos personalizados de MPI y el uso de las funciones de topología cartesiana de MPI
- La escritura de aplicaciones con MPI híbrido más OpenMP

La importancia del estándar Message Passing Interface (MPI) radica en que permite a un programa acceder a nodos de cómputo adicionales y, por lo tanto, ejecutar problemas cada vez más grandes añadiendo más nodos a la simulación. El nombre *paso de mensajes* (message passing) se refiere a la capacidad de enviar fácilmente mensajes de un proceso a otro. MPI es omnipresente en el campo del cómputo de alto rendimiento. En muchos campos científicos, el uso de supercomputadoras implica una implementación de MPI.

MPI se lanzó como un estándar abierto en 1994 y, en cuestión de meses, se convirtió en el lenguaje basado en bibliotecas dominante para el cómputo paralelo. Desde 1994, el uso de MPI ha dado lugar a avances científicos que van desde la física hasta el aprendizaje automático y los automóviles autónomos. En la actualidad, varias implementaciones de MPI son de uso generalizado. MPICH, del Argonne National Laboratories, y OpenMPI son dos de las más comunes. Los fabricantes de hardware suelen tener versiones personalizadas de una de estas dos implementaciones para sus plataformas. El estándar MPI, que a fecha de 2015 alcanza ya la versión 3.1, continúa evolucionando y cambiando.

En este capítulo, le mostraremos cómo implementar MPI en su aplicación. Comenzaremos con un programa MPI simple y luego avanzaremos hacia un ejemplo más complicado de cómo enlazar mallas computacionales separadas, situadas en procesos separados, mediante la comunicación de información de frontera. Abordaremos algunas técnicas avanzadas que son importantes para programas MPI bien escritos, como la construcción de tipos de datos personalizados de MPI y el uso de las funciones de topología cartesiana de MPI. Por último, presentaremos la combinación de MPI con OpenMP (MPI más OpenMP) y la vectorización para obtener múltiples niveles de paralelismo.

> **NOTA** Le animamos a seguir los ejemplos de este capítulo en <https://github.com/EssentialsofParallelComputing/Chapter8>.

## 8.1 Los fundamentos de un programa MPI

En esta sección cubriremos los fundamentos necesarios para un programa MPI mínimo. Algunos de estos requisitos básicos están especificados por el estándar MPI, mientras que otros los proporciona por convención la mayoría de las implementaciones de MPI. La estructura y el funcionamiento básicos de MPI se han mantenido notablemente consistentes desde el primer estándar.

Para empezar, MPI es un lenguaje completamente basado en bibliotecas. No requiere un compilador especial ni adaptaciones por parte del sistema operativo. Todos los programas MPI tienen una estructura y un proceso básicos, como muestra la figura 8.1. MPI siempre comienza con una llamada a `MPI_Init` justo al inicio del programa y con una llamada a `MPI_Finalize` a la salida del programa. Esto contrasta con OpenMP, como se explicó en el capítulo 7, que no necesita comandos especiales de arranque y apagado y simplemente coloca directivas paralelas alrededor de los bucles clave.

> **Figura 8.1** El enfoque de MPI está basado en bibliotecas. Basta con compilar, enlazando la biblioteca de MPI, y lanzar el programa con un programa de arranque paralelo especial.

Contenido de la figura 8.1:

**Escribir el programa MPI:**

```c
#include <mpi.h>
int main(int argc, char *argv[])
{
MPI_Init(&argc, &argv);
MPI_Finalize();
return(0);
}
```

**Compilar:**

- Wrappers: `mpicc`, `mpiCC`, `mpif90`
- o bien, de forma manual: incluir `mpi.h` y enlazar la biblioteca de MPI

**Ejecutar:**

```bash
mpirun -n <#procs> my_prog.x
```

Nombres alternativos para `mpirun`: `mpiexec`, `aprun`, `srun`.

Una vez que escribe un programa paralelo MPI, este se compila con un archivo de inclusión (include) y una biblioteca. Luego se ejecuta con un programa de arranque especial que establece los procesos paralelos entre nodos y dentro del nodo.

### 8.1.1 Llamadas básicas de funciones MPI para todo programa MPI

Las llamadas básicas de funciones MPI incluyen `MPI_Init` y `MPI_Finalize`. La llamada a `MPI_Init` debe realizarse justo después del arranque del programa, y los argumentos de la rutina `main` deben pasarse a la llamada de inicialización. Las llamadas típicas tienen el siguiente aspecto y pueden o no usar la variable de retorno:

```c
iret = MPI_Init(&argc, &argv);
iret = MPI_Finalize();
```

La mayoría de los programas necesitarán el número de procesos y el rank del proceso dentro del grupo que puede comunicarse, denominado *comunicador* (communicator). Una de las funciones principales de MPI es iniciar procesos remotos y enlazarlos para que se puedan enviar mensajes entre ellos. El comunicador por defecto es `MPI_COMM_WORLD`, que se configura al inicio de cada trabajo paralelo mediante `MPI_Init`. Tomemos un momento para revisar algunas definiciones:

- **Proceso (Process)**: una unidad de cómputo independiente que posee una porción de memoria y el control sobre los recursos en el espacio de usuario.
- **Rank**: un identificador único y portable para distinguir el proceso individual dentro del conjunto de procesos. Normalmente sería un entero dentro del conjunto de enteros desde cero hasta el número de procesos menos uno.

Las llamadas para obtener estas importantes variables son:

```c
iret = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
iret = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
```

### 8.1.2 Wrappers del compilador para programas MPI más sencillos

Aunque MPI es una biblioteca, podemos tratarla como un compilador mediante el uso de los wrappers (envoltorios) del compilador de MPI. Esto facilita la construcción de aplicaciones MPI porque no necesita saber qué bibliotecas se requieren ni dónde están ubicadas. Estos resultan especialmente cómodos para aplicaciones MPI pequeñas. Existe un wrapper de compilador para cada lenguaje de programación:

- `mpicc` — Wrapper para código C
- `mpicxx` — Wrapper para C++ (también puede ser `mpiCC` o `mpic++`)
- `mpifort` — Wrapper para Fortran (también puede ser `mpif77` o `mpif90`)

El uso de estos wrappers es opcional. Si no utiliza los wrappers del compilador, estos aún pueden ser valiosos para identificar las banderas (flags) de compilación necesarias para construir su aplicación. El comando `mpicc` tiene opciones que muestran esta información. Puede encontrar estas opciones para su MPI con `man mpicc`. Para las dos implementaciones de MPI más populares, listamos aquí las opciones de línea de comandos para `mpicc`, `mpicxx` y `mpifort`.

Para OpenMPI, use estas opciones de comando:

- `--showme`
- `--showme:compile`
- `--showme:link`

Para MPICH, use estas opciones de comando:

- `-show`
- `-compile_info`
- `-link_info`

### 8.1.3 Uso de los comandos de arranque paralelo

El arranque de los procesos paralelos de MPI es una operación compleja que gestiona un comando especial. Al principio, este comando solía ser `mpirun`. Pero con la publicación del estándar MPI 2.0 en 1997, se recomendó que el comando de arranque fuera `mpiexec`, para intentar proporcionar mayor portabilidad. Sin embargo, este intento de estandarización no tuvo un éxito completo, y hoy en día existen varios nombres para el comando de arranque:

- `mpirun -n <nprocs>`
- `mpiexec -n <nprocs>`
- `aprun`
- `srun`

La mayoría de los comandos de arranque de MPI aceptan la opción `-n` para el número de procesos, pero otros podrían aceptar `-np`. Con la complejidad de las arquitecturas de nodos de cómputo recientes, los comandos de arranque tienen una miríada de opciones para afinidad, ubicación (placement) y entorno (algunas de las cuales discutiremos en el capítulo 14). Estas opciones varían con cada implementación de MPI e incluso con cada versión de sus bibliotecas MPI. La simplicidad de las opciones disponibles en los comandos de arranque originales se ha transformado en una maraña confusa de opciones que aún no se ha estabilizado. Afortunadamente, para el usuario principiante de MPI, la mayoría de estas opciones pueden ignorarse, pero son importantes para el uso avanzado y el ajuste (tuning).

### 8.1.4 Ejemplo mínimo funcional de un programa MPI

Ahora que hemos aprendido todos los componentes básicos, podemos combinarlos en el ejemplo mínimo funcional que muestra el listado 8.1: iniciamos el trabajo paralelo e imprimimos el rank y el número de procesos desde cada proceso. En la llamada para obtener el rank y el tamaño, usamos la variable `MPI_COMM_WORLD`, que es el grupo de todos los procesos MPI y está predefinida en el archivo de cabecera de MPI. Tenga en cuenta que la salida mostrada puede aparecer en cualquier orden; el programa MPI deja en manos del sistema operativo cuándo y cómo se muestra la salida.

**Listado 8.1 — Ejemplo mínimo funcional de MPI**

`MinWorkExampleMPI.c`

```c
 1 #include <mpi.h>
 2 #include <stdio.h>
 3 int main(int argc, char **argv)
 4 {
 5 MPI_Init(&argc, &argv);
 6
 7 int rank, nprocs;
 8 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 9 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
10
11 printf("Rank %d of %d\n", rank, nprocs);
12
13 MPI_Finalize();
14 return 0;
15 }
```

Anotaciones del listado:

- Línea 1: archivo include para las funciones y variables de MPI.
- Línea 5 (`MPI_Init`): inicializa tras el arranque del programa, incluyendo los argumentos del programa.
- Línea 8 (`MPI_Comm_rank`): obtiene el número de rank del proceso.
- Línea 9 (`MPI_Comm_size`): obtiene el número de ranks del programa, determinado por el comando `mpirun`.
- Línea 13 (`MPI_Finalize`): finaliza MPI para sincronizar los ranks y luego sale.

El listado 8.2 define un makefile simple para construir este ejemplo usando los wrappers del compilador de MPI. En este caso, usamos el wrapper `mpicc` para proporcionar la ubicación del archivo include `mpi.h` y la biblioteca de MPI.

**Listado 8.2 — Makefile simple usando los wrappers del compilador de MPI**

`MinWorkExample/Makefile.simple`

```makefile
default: MinWorkExampleMPI
all: MinWorkExampleMPI
MinWorkExampleMPI: MinWorkExampleMPI.c Makefile
	mpicc MinWorkExampleMPI.c -o MinWorkExampleMPI
clean:
	rm -f MinWorkExampleMPI MinWorkExampleMPI.o
```

Para construcciones más elaboradas en una variedad de sistemas, podría preferir CMake. El siguiente listado muestra el archivo CMakeLists.txt para este programa.

**Listado 8.3 — El CMakeLists.txt para construir con CMake**

`MinWorkExample/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 2.8)
project(MinWorkExampleMPI)
# Require MPI for this project:
find_package(MPI REQUIRED)
add_executable(MinWorkExampleMPI MinWorkExampleMPI.c)

target_include_directories(MinWorkExampleMPI
    PRIVATE ${MPI_C_INCLUDE_PATH})
target_compile_options(MinWorkExampleMPI
    PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(MinWorkExampleMPI
    ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS})
# Add a test:
enable_testing()
add_test(MPITest ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG}
    ${MPIEXEC_MAX_NUMPROCS}
    ${MPIEXEC_PREFLAGS}
    ${CMAKE_CURRENT_BINARY_DIR}/MinWorkExampleMPI
    ${MPIEXEC_POSTFLAGS})
# Cleanup
add_custom_target(distclean COMMAND rm -rf CMakeCache.txt CMakeFiles
    Makefile cmake_install.cmake CTestTestfile.cmake Testing)
```

Anotaciones del listado:

- `find_package(MPI REQUIRED)`: llama a un módulo especial para encontrar MPI y establece las variables.
- `target_compile_options(...)`: modifica las banderas (flags) de compilación.
- `add_test(...)`: crea una prueba (test) de MPI portable.

Ahora, usando el sistema de construcción CMake, configuremos, construyamos y luego ejecutemos la prueba con estos comandos:

```bash
cmake .
make
make test
```

La operación de escritura del comando `printf` muestra la salida en cualquier orden. Finalmente, para limpiar después de la ejecución, use estos comandos:

```bash
make clean
make distclean
```

## 8.2 Los comandos send y receive para la comunicación proceso a proceso

El núcleo del enfoque de paso de mensajes consiste en enviar un mensaje de punto a punto o, quizás más precisamente, de proceso a proceso. Todo el propósito del procesamiento paralelo es coordinar el trabajo. Para ello, es necesario enviar mensajes, ya sea para el control o para la distribución del trabajo. Le mostraremos cómo se componen y se envían correctamente estos mensajes. Existen muchas variantes de las rutinas punto a punto; cubriremos aquellas cuyo uso se recomienda en la mayoría de las situaciones.

La figura 8.2 muestra los componentes de un mensaje. Debe haber un buzón (mailbox) en cada extremo del sistema. El tamaño del buzón es importante. El lado emisor conoce el tamaño del mensaje, pero el lado receptor no. Para asegurarse de que haya un lugar donde almacenar el mensaje, suele ser mejor publicar la recepción (post the receive) primero. Esto evita retrasar el mensaje por el hecho de que el proceso receptor tenga que asignar un espacio temporal para almacenar el mensaje hasta que se publique una recepción y pueda copiarlo a la ubicación correcta. Como analogía, si la recepción (el buzón) no está publicada (no está ahí), el cartero tiene que esperar hasta que alguien instale uno. Publicar la recepción primero evita la posibilidad de que no haya suficiente espacio de memoria en el extremo receptor para asignar un buffer temporal donde almacenar el mensaje.

> **Figura 8.2** Un mensaje en MPI siempre se compone de un puntero a memoria, un conteo (count) y un tipo (type). El sobre (envelope) tiene una dirección compuesta por un rank, un tag y un grupo de comunicación, junto con un contexto interno de MPI.

Elementos de la figura 8.2:

- Buzón de envío con el mensaje listo para partir.
- Buzón de recepción con espacio disponible para la recepción.
- El mensaje contiene: puntero a memoria, conteo (count) y tipo de dato (data type).
- El sobre (envelope) contiene la dirección: Rank, Tag y Grupo de comunicación (Comm Group).

El mensaje en sí siempre se compone de un triplete en ambos extremos: un puntero a un buffer de memoria, un conteo (count) y un tipo (type). El tipo enviado y el tipo recibido pueden ser tipos y conteos diferentes. La razón para usar tipos y conteos es que permite la conversión de tipos entre los procesos del origen y del destino. Esto permite convertir un mensaje a una forma diferente en el extremo receptor. En un entorno heterogéneo, esto podría significar convertir de lower-endian a big-endian, una diferencia de bajo nivel en el orden de los bytes de los datos almacenados en hardware de distintos fabricantes. Además, el tamaño de recepción puede ser mayor que la cantidad enviada. Esto permite al receptor consultar cuántos datos se envían, para poder manejar el mensaje adecuadamente. Pero el tamaño de recepción no puede ser menor que el tamaño de envío, porque provocaría una escritura más allá del final del buffer.

El sobre (envelope) también se compone de un triplete. Define de quién es el mensaje, a quién se envía y un identificador de mensaje para evitar confundir varios mensajes entre sí. El triplete consta del rank, el tag y el grupo de comunicación. El rank corresponde al grupo de comunicación especificado. El tag ayuda al programador y a MPI a distinguir qué mensaje corresponde a qué recepción. En MPI, el tag es una conveniencia. Puede establecerse en `MPI_ANY_TAG` si no se desea un número de tag explícito. MPI utiliza un contexto creado internamente dentro de la biblioteca para separar correctamente los mensajes. Tanto el comunicador como el tag deben coincidir para que un mensaje se complete.

> **NOTA** Una de las fortalezas del enfoque de paso de mensajes es el modelo de memoria. Cada proceso tiene una propiedad clara de sus datos, además del control y la sincronización sobre cuándo cambian dichos datos. Tiene la garantía de que ningún otro proceso puede cambiar su memoria a sus espaldas.

Ahora probemos un programa MPI con un send/receive simple. Tenemos que enviar datos en un proceso y recibir datos en otro. Hay diferentes maneras en las que podríamos emitir estas llamadas en un par de procesos (figura 8.3). Algunas de las combinaciones de send y receive básicos con bloqueo (blocking) no son seguras y pueden colgarse, como las dos combinaciones de la izquierda de la figura 8.3. La tercera combinación requiere una programación cuidadosa con condicionales. El método del extremo derecho es uno de varios métodos seguros para planificar las comunicaciones usando sends y receives non-blocking. Estas también se denominan llamadas asíncronas o inmediatas (immediate), lo que explica el carácter `I` que precede a las palabras clave send y receive (el caso mostrado en el extremo derecho de la figura).

> **Figura 8.3** El orden de los send y receive blocking es difícil de hacer correctamente. Es mucho más seguro y rápido usar las formas non-blocking o inmediatas de las operaciones de send y receive, y luego esperar a su finalización.

Contenido de la figura 8.3 (cuatro casos):

- **Llamadas de comunicación blocking:**
  - *Receive blocking primero* (Rank 0 y Rank 1): bloqueado, esperando datos.
  - *Send blocking primero* (Rank 0 y Rank 1): bloqueado, esperando datos.
  - *Send y receive alternados* (Rank 0 y Rank 1): si el buffer está disponible, copia y continúa.
- **Llamadas de comunicación non-blocking:**
  - *Send/receive non-blocking* (Rank 0 y Rank 1): `Isend`/`Irecv` seguidos de `Waitall`. Todas las operaciones se publican y luego se espera a su finalización; cuando la operación se completa, se puede continuar.

El send y receive más básicos de MPI son `MPI_Send` y `MPI_Recv`. Las funciones básicas de send y receive tienen los siguientes prototipos:

```c
MPI_Send(void *data, int count, MPI_Datatype datatype, int dest, int tag,
         MPI_COMM comm)
MPI_Recv(void *data, int count, MPI_Datatype datatype, int source, int tag,
         MPI_COMM comm, MPI_Status *status)
```

Ahora repasemos cada uno de los cuatro casos de la figura 8.3 para entender por qué algunos se cuelgan y otros funcionan bien. Comenzaremos con el `MPI_Send` y el `MPI_Recv` que se mostraron en los prototipos de función anteriores y en el ejemplo más a la izquierda de la figura. Ambas rutinas son blocking. Blocking significa que no retornan hasta que se cumple una condición específica. En el caso de estas dos llamadas, la condición para retornar es que el buffer sea seguro para volver a usarse. En el send, el buffer debe haber sido leído y ya no ser necesario. En el receive, el buffer debe estar lleno. Si ambos procesos de una comunicación están bloqueando (blocking), puede ocurrir una situación conocida como cuelgue (hang). Un cuelgue ocurre cuando uno o más procesos están esperando un evento que nunca puede ocurrir.

### Ejemplo: un programa send/receive con bloqueo que se cuelga

Este ejemplo destaca un problema común en la programación paralela. Siempre debe estar en guardia para evitar una situación que pueda colgarse (deadlock). En el siguiente listado, vemos cómo podría ocurrir esto para poder evitarlo.

`Send_Recv/SendRecv1.c`

```c
 1 #include <mpi.h>
 2 #include <stdio.h>
 3 #include <stdlib.h>
 4 int main(int argc, char **argv)
 5 {
 6 MPI_Init(&argc, &argv);
 7
 8 int count = 10;
 9 double xsend[count], xrecv[count];
10 for (int i=0; i<count; i++){
11 xsend[i] = (double)i;
12 }
13
14 int rank, nprocs;
15 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
16 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
17 if (nprocs%2 == 1){
18 if (rank == 0){
19 printf("Must be called with an even number of processes\n");
20 }
21 exit(1);
22 }
23
24 int tag = rank/2;
25 int partner_rank = (rank/2)*2 + (rank+1)%2;
26 MPI_Comm comm = MPI_COMM_WORLD;
27
28 MPI_Recv(xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, MPI_STATUS_IGNORE);
29 MPI_Send(xsend, count, MPI_DOUBLE, partner_rank, tag, comm);
30
31 if (rank == 0) printf("SendRecv successfully completed\n");
32
33 MPI_Finalize();
34 return 0;
35 }
```

*Un ejemplo simple de send/receive en MPI (siempre se cuelga).*

Anotaciones del listado:

- Línea 24 (`tag`): la división entera empareja los tags de los socios (partners) de send y receive.
- Línea 25 (`partner_rank`): el rank del socio es el otro miembro del par.
- Línea 28 (`MPI_Recv`): las recepciones se publican primero.
- Línea 29 (`MPI_Send`): los envíos se realizan después de las recepciones.

El tag y el rank del socio de comunicación se calculan mediante aritmética entera y modular, que empareja los tags para cada send y receive y obtiene el rank del otro miembro del par. Luego se publican las recepciones para cada proceso con su socio. Estas son recepciones blocking que no se completan (no retornan) hasta que el buffer se llena. Como el send no se llama hasta después de que las recepciones se completen, el programa se cuelga. Observe que escribimos las llamadas de send y receive sin sentencias `if` (condicionales) basadas en el rank. Los condicionales son la fuente de muchos errores en el código paralelo, por lo que generalmente conviene evitarlos.

Probemos invirtiendo el orden de los sends y receives. Listamos las líneas modificadas en el siguiente listado a partir del listado original del ejemplo anterior.

**Listado 8.4 — Un ejemplo simple de send/receive en MPI (a veces falla)**

`Send_Recv/SendRecv2.c`

```c
28 MPI_Send(xsend, count, MPI_DOUBLE, partner_rank, tag, comm);
29 MPI_Recv(xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, MPI_STATUS_IGNORE);
```

Anotaciones del listado:

- Línea 28: primero llama a la operación de send.
- Línea 29: luego llama a la operación de receive, después de que el send se completa.

Entonces, ¿este también falla? Bueno, depende. La llamada send retorna después de que se completa el uso del buffer de datos de envío. La mayoría de las implementaciones de MPI copiarán los datos en buffers preasignados en el emisor o el receptor si el tamaño es suficientemente pequeño. En este caso, el send se completa y se llama al receive. Si el mensaje es grande, el send espera a que la llamada receive asigne un buffer donde colocar el mensaje antes de retornar. Pero el receive nunca se llama, así que el programa se cuelga. Podríamos alternar la publicación de sends y receives por ranks para que no ocurran los cuelgues. Tenemos que usar un condicional para esta variante, como muestra el siguiente listado.

**Listado 8.5 — Send/receive con sends y receives alternados por rank**

`Send_Recv/SendRecv3.c`

```c
28 if (rank%2 == 0) {
29 MPI_Send(xsend, count, MPI_DOUBLE, partner_rank, tag, comm);
30 MPI_Recv(xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, MPI_STATUS_IGNORE);
31 } else {
32 MPI_Recv(xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, MPI_STATUS_IGNORE);
33 MPI_Send(xsend, count, MPI_DOUBLE, partner_rank, tag, comm);
34 }
```

Anotaciones del listado:

- Línea 28: los ranks pares publican el send primero.
- Línea 31 (`else`): los ranks impares hacen el receive primero.

Pero esto es complicado de hacer correctamente en comunicaciones más complejas y requiere un uso cuidadoso de condicionales. Una mejor manera de implementar esto es usando la llamada `MPI_Sendrecv`, como muestra el siguiente listado. Al usar esta llamada, transfiere la responsabilidad de ejecutar correctamente la comunicación a la biblioteca de MPI. Este es un trato bastante bueno para el programador.

**Listado 8.6 — Send/receive con la llamada MPI_Sendrecv**

`Send_Recv/SendRecv4.c`

```c
28 MPI_Sendrecv(xsend, count, MPI_DOUBLE, partner_rank, tag,
29              xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, MPI_STATUS_IGNORE);
```

Anotaciones del listado:

- Una llamada combinada de send/receive reemplaza las llamadas individuales `MPI_Send` y `MPI_Recv`.

La llamada `MPI_Sendrecv` es un buen ejemplo de las ventajas de usar las llamadas de comunicación colectiva que presentaremos en la sección 8.3. Es buena práctica usar las llamadas de comunicación colectiva siempre que sea posible, porque delegan en la biblioteca de MPI la responsabilidad de evitar cuelgues y deadlocks, así como la responsabilidad de un buen rendimiento.

Como alternativa a las llamadas de comunicación blocking de los ejemplos anteriores, veamos el uso de `MPI_Isend` y `MPI_Irecv` en el listado 8.7. Estas se denominan versiones inmediatas (immediate, `I`) porque retornan inmediatamente. Esto se suele denominar llamadas asíncronas o non-blocking. Asíncrono significa que la llamada inicia la operación pero no espera a que el trabajo se complete.

**Listado 8.7 — Un ejemplo simple de send/receive usando Isend e Irecv**

`Send_Recv/SendRecv5.c`

```c
27 MPI_Request requests[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
28
29 MPI_Irecv(xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, &requests[0]);
30 MPI_Isend(xsend, count, MPI_DOUBLE, partner_rank, tag, comm, &requests[1]);
31 MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);
```

Anotaciones del listado:

- Línea 27: define un arreglo de requests y los establece en null para que estén definidos cuando se comprueba su finalización.
- Línea 29 (`MPI_Irecv`): el `Irecv` se publica primero.
- Línea 30 (`MPI_Isend`): el `Isend` se llama a continuación, tras el `Irecv`.
- Línea 31 (`MPI_Waitall`): llama a un `Waitall` para esperar a que el send y el receive se completen.

Cada proceso espera en `MPI_Waitall`, en la línea 31 del listado, a que se complete el mensaje. También debería ver una mejora medible en el rendimiento del programa al reducir el número de puntos que bloquean, pasando de cada llamada de send y receive a un único `MPI_Waitall`. Pero debe tener cuidado de no modificar el buffer de envío ni acceder al buffer de recepción hasta que la operación se complete. Hay otras combinaciones que funcionan. Veamos el siguiente listado, que usa una de las posibilidades.

**Listado 8.8 — Un ejemplo mixto de send/receive inmediato y con bloqueo**

`Send_Recv/SendRecv6.c`

```c
27 MPI_Request request;
28
29 MPI_Isend(xsend, count, MPI_DOUBLE, partner_rank, tag, comm, &request);
30 MPI_Recv(xrecv, count, MPI_DOUBLE, partner_rank, tag, comm, MPI_STATUS_IGNORE);
31 MPI_Request_free(&request);
```

Anotaciones del listado:

- Línea 29 (`MPI_Isend`): publica el send con un `MPI_Isend` para que retorne (inmediatamente).
- Línea 30 (`MPI_Recv`): llama al receive blocking. Este proceso puede continuar tan pronto como retorne.
- Línea 31 (`MPI_Request_free`): libera el handle de la petición (request) para evitar una fuga de memoria.

Iniciamos la comunicación con un send asíncrono y luego bloqueamos con un receive blocking. Una vez que el receive blocking se completa, este proceso puede continuar incluso si el send no se ha completado. Aún debe liberar el handle de la petición (request) con `MPI_Request_free`, o como efecto secundario de una llamada a `MPI_Wait` o `MPI_Test`, para evitar una fuga de memoria. También puede llamar a `MPI_Request_free` inmediatamente después del `MPI_Isend`.

Otras variantes de send/receive podrían ser útiles en situaciones especiales. Los modos se indican mediante un prefijo de una o dos letras, similar al que se ve en la variante inmediata, como se lista aquí:

- `B` (buffered) — almacenado en buffer
- `S` (synchronous) — síncrono
- `R` (ready) — listo
- `IB` (immediate buffered) — inmediato almacenado en buffer
- `IS` (immediate synchronous) — inmediato síncrono
- `IR` (immediate ready) — inmediato listo

La lista de tipos de datos de MPI predefinidos para C es extensa; los tipos de datos se corresponden con casi todos los tipos del lenguaje C. MPI también tiene tipos correspondientes a los tipos de datos de Fortran. Listamos solo los más comunes para C:

- `MPI_CHAR` (un tipo carácter de C de 1 byte)
- `MPI_INT` (un tipo entero de 4 bytes)
- `MPI_FLOAT` (un tipo real de 4 bytes)
- `MPI_DOUBLE` (un tipo real de 8 bytes)
- `MPI_PACKED` (un tipo genérico de tamaño byte, usado normalmente para tipos de datos mixtos)
- `MPI_BYTE` (un tipo genérico de tamaño byte)

`MPI_PACKED` y `MPI_BYTE` son tipos especiales y coinciden con cualquier otro tipo. `MPI_BYTE` indica un valor sin tipo y el conteo (count) especifica el número de bytes. Evita cualquier operación de conversión de datos en comunicaciones de datos heterogéneas. `MPI_PACKED` se usa con la rutina `MPI_Pack`, como muestra el ejemplo de ghost exchange (intercambio de celdas fantasma) en la sección 8.4.3. También puede definir su propio tipo de datos para usar en estas llamadas. Esto también se demuestra en el ejemplo de ghost exchange. Existen además muchas rutinas de comprobación de finalización de la comunicación, que incluyen:

```c
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
int MPI_Testany(int count, MPI_Request requests[], int *index, int *flag,
                MPI_Status *status)
int MPI_Testall(int count, MPI_Request requests[], int *flag,
                MPI_Status statuses[])
int MPI_Testsome(int incount, MPI_Request requests[], int *outcount,
                 int indices[], MPI_Status statuses[])
int MPI_Wait(MPI_Request *request, MPI_Status *status)
int MPI_Waitany(int count, MPI_Request requests[], int *index,
                MPI_Status *status)
int MPI_Waitall(int count, MPI_Request requests[], MPI_Status statuses[])
int MPI_Waitsome(int incount, MPI_Request requests[], int *outcount,
                 int indices[], MPI_Status statuses[])
int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
```

Existen variantes adicionales de `MPI_Probe` que no se listan aquí. `MPI_Waitall` se muestra en varios ejemplos de este capítulo. Las demás rutinas son útiles en situaciones más especializadas. El nombre de las rutinas da una buena idea de las capacidades que proporcionan.


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


## 8.5 Funcionalidad avanzada de MPI para simplificar el código y habilitar optimizaciones

El excelente diseño de MPI se hace evidente cuando vemos cómo los componentes básicos de MPI pueden combinarse en funcionalidad de más alto nivel. Tuvimos una muestra de esto en la sección 8.3.3, cuando creamos un nuevo tipo double-double y un nuevo operador de reducción. Esta extensibilidad le da a MPI capacidades importantes. Veremos un par de estas funciones avanzadas que son útiles en aplicaciones comunes de paralelismo de datos. Estas incluyen:

- **Tipos de datos personalizados de MPI** (MPI custom data types): construye nuevos tipos de datos a partir de los bloques de construcción de los tipos básicos de MPI.
- **Soporte de topología** (Topology support): están disponibles tanto una topología básica de cuadrícula regular cartesiana como una topología de grafo más general. Solo veremos las funciones cartesianas de MPI, que son más simples.

### 8.5.1 Uso de tipos de datos personalizados de MPI para el rendimiento y la simplificación del código

MPI tiene un rico conjunto de funciones para crear nuevos tipos de datos personalizados de MPI a partir de los tipos básicos de MPI. Esto permite encapsular datos complejos en un único tipo de datos personalizado que puede usar en las llamadas de comunicación. Como resultado, una única llamada de comunicación puede enviar o recibir muchos fragmentos de datos más pequeños como una unidad. Aquí hay una lista de algunas de las funciones de creación de tipos de datos de MPI:

- `MPI_Type_contiguous` — Convierte un bloque de datos contiguos en un tipo.
- `MPI_Type_vector` — Crea un tipo a partir de bloques de datos con paso (strided).
- `MPI_Type_create_subarray` — Crea un subconjunto rectangular de un arreglo más grande.
- `MPI_Type_indexed` o `MPI_Type_create_hindexed` — Crea un conjunto irregular de índices descrito por un conjunto de longitudes de bloque y desplazamientos. La versión hindexed expresa los desplazamientos en bytes en lugar de en un tipo de datos, para mayor generalidad.
- `MPI_Type_create_struct` — Crea un tipo de datos que encapsula los elementos de datos de una estructura de manera portable, teniendo en cuenta el relleno (padding) del compilador.

Encontrará útil una ilustración visual para entender algunos de estos tipos de datos. La figura 8.6 muestra algunas de las funciones más simples y más comúnmente usadas, incluyendo `MPI_Type_contiguous`, `MPI_Type_vector` y `MPI_Type_create_subarray`.

> **Figura 8.6** Tres tipos de datos personalizados de MPI con ilustraciones de los argumentos usados en su creación.

Contenido de la figura 8.6:

- **`MPI_Type_contiguous`**: un número (Count) de elementos de datos contiguos formando un solo bloque.
- **`MPI_Type_vector`**: bloques de datos con paso (stride). Se definen mediante una longitud de bloque (Block length), un paso (Stride) entre el inicio de un bloque y el siguiente, y un número de bloques (Count) — en la ilustración, los bloques 1, 2 y 3.
- **`MPI_Type_create_subarray`**: un subconjunto rectangular dentro de un arreglo más grande. En la ilustración, `Array sizes = {7,10}` (tamaño del arreglo completo), `Subsizes = {3,4}` (tamaño del subarreglo) y `Start sizes = {2,2}` (posición de inicio del subarreglo).

Una vez que un tipo de datos se describe y se convierte en un nuevo tipo de datos, debe inicializarse antes de usarse. Para este propósito, hay un par de rutinas adicionales para confirmar (commit) y liberar (free) los tipos. Un tipo debe confirmarse antes de usarse y debe liberarse para evitar una fuga de memoria. Las rutinas incluyen:

- `MPI_Type_Commit` — Inicializa el nuevo tipo personalizado con la asignación de memoria necesaria u otra configuración.
- `MPI_Type_Free` — Libera cualquier memoria o entradas de estructuras de datos creadas durante la creación del tipo de datos.

Podemos simplificar enormemente la comunicación de ghost cells definiendo un tipo de datos personalizado de MPI, como se mostró en la figura 8.6, para representar la columna de datos y evitar las llamadas `MPI_Pack`. Al definir un tipo de datos de MPI, se puede evitar una copia de datos adicional. Los datos pueden copiarse desde su ubicación normal directamente a los buffers de envío de MPI. Veamos cómo se hace esto en el listado 8.23. El listado 8.24 muestra la segunda parte del programa.

Primero configuramos los tipos de datos personalizados. Usamos la llamada `MPI_Type_vector` para conjuntos de accesos a arreglos con paso (strided). Para los datos contiguos del tipo vertical cuando incluimos esquinas, usamos la llamada `MPI_Type_contiguous`, y en las líneas 139 y 140 liberamos el tipo de datos al final, antes de `MPI_Finalize`.

**Listado 8.23 — Creación de un tipo de datos vector 2D para la actualización de ghost cells**

`GhostExchange/GhostExchange_VectorTypes/GhostExchange.cc`

```cpp
 56 int jlow=0, jhgh=jsize;
 57 if (corners) {
 58 if (nbot == MPI_PROC_NULL) jlow = -nhalo;
 59 if (ntop == MPI_PROC_NULL) jhgh = jsize+nhalo;
 60 }
 61 int jnum = jhgh-jlow;
 62
 63 MPI_Datatype horiz_type;
 64 MPI_Type_vector(jnum, nhalo, isize+2*nhalo, MPI_DOUBLE, &horiz_type);
 65 MPI_Type_commit(&horiz_type);
 66
 67 MPI_Datatype vert_type;
 68 if (! corners){
 69 MPI_Type_vector(nhalo, isize, isize+2*nhalo, MPI_DOUBLE, &vert_type);
 70 } else {
 71 MPI_Type_contiguous(nhalo*(isize+2*nhalo), MPI_DOUBLE, &vert_type);
 72 }
 73 MPI_Type_commit(&vert_type);
...
139 MPI_Type_free(&horiz_type);
140 MPI_Type_free(&vert_type);
```

Luego puede escribir `ghostcell_update` de forma más concisa y con mejor rendimiento usando los tipos de datos de MPI, como en el siguiente listado. Si necesitamos actualizar las esquinas, se requiere una sincronización entre los dos pases de comunicación.

**Listado 8.24 — Rutina de actualización de ghost cells 2D usando el tipo de datos vector**

`GhostExchange/GhostExchange_VectorTypes/GhostExchange.cc`

```cpp
197 int jlow=0, jhgh=jsize, ilow=0, waitcount=8, ib=4;
198 if (corners) {
199 if (nbot == MPI_PROC_NULL) jlow = -nhalo;
200 ilow = -nhalo;
201 waitcount = 4;
202 ib = 0;
203 }
204
205 MPI_Request request[waitcount];
206 MPI_Status status[waitcount];
207
208 MPI_Irecv(&x[jlow][isize], 1, horiz_type, nrght, 1001,
209 MPI_COMM_WORLD, &request[0]);
210 MPI_Isend(&x[jlow][0], 1, horiz_type, nleft, 1001,
211 MPI_COMM_WORLD, &request[1]);
212
213 MPI_Irecv(&x[jlow][-nhalo], 1, horiz_type, nleft, 1002,
214 MPI_COMM_WORLD, &request[2]);
215 MPI_Isend(&x[jlow][isize-nhalo], 1, horiz_type, nrght, 1002,
216 MPI_COMM_WORLD, &request[3]);
217
218 if (corners) MPI_Waitall(4, request, status);
219
220 MPI_Irecv(&x[jsize][ilow], 1, vert_type, ntop, 1003,
221 MPI_COMM_WORLD, &request[ib+0]);
222 MPI_Isend(&x[0 ][ilow], 1, vert_type, nbot, 1003,
223 MPI_COMM_WORLD, &request[ib+1]);
224
225 MPI_Irecv(&x[ -nhalo][ilow], 1, vert_type, nbot, 1004,
226 MPI_COMM_WORLD, &request[ib+2]);
227 MPI_Isend(&x[jsize-nhalo][ilow], 1, vert_type, ntop, 1004,
228 MPI_COMM_WORLD, &request[ib+3]);
229
230 MPI_Waitall(waitcount, request, status);
```

Anotaciones del listado:

- Líneas 208–216: envía a izquierda y derecha usando el tipo de datos personalizado de MPI `horiz_type`.
- Línea 218 (`if (corners) MPI_Waitall`): sincroniza si se envían las esquinas.
- Líneas 220–228: actualiza las ghost cells arriba y abajo.

La razón para usar tipos de datos de MPI suele darse como un mejor rendimiento. Sí permite que la implementación de MPI evite una copia adicional en algunos casos. Pero, desde nuestra perspectiva, la mayor razón para usar tipos de datos de MPI es el código más limpio y simple, y menos oportunidades para errores (bugs).

La versión 3D que usa tipos de datos de MPI es un poco más complicada. Usamos `MPI_Type_create_subarray` en el siguiente listado para crear tres tipos de datos personalizados de MPI que se usarán en la comunicación.

**Listado 8.25 — Creación de un tipo de datos subarray de MPI para ghost cells 3D**

`GhostExchange/GhostExchange3D_VectorTypes/GhostExchange.cc`

```cpp
109 int array_sizes[] = {ksize+2*nhalo, jsize+2*nhalo, isize+2*nhalo};
110 if (corners) {
111 int subarray_starts[] = {0, 0, 0};
112 int hsubarray_sizes[] = {ksize+2*nhalo, jsize+2*nhalo, nhalo};
113 MPI_Type_create_subarray(3, array_sizes, hsubarray_sizes,
114 subarray_starts, MPI_ORDER_C, MPI_DOUBLE, &horiz_type);
115
116 int vsubarray_sizes[] = {ksize+2*nhalo, nhalo, isize+2*nhalo};
117 MPI_Type_create_subarray(3, array_sizes, vsubarray_sizes,
118 subarray_starts, MPI_ORDER_C, MPI_DOUBLE, &vert_type);
119
120 int dsubarray_sizes[] = {nhalo, jsize+2*nhalo, isize+2*nhalo};
121 MPI_Type_create_subarray(3, array_sizes, dsubarray_sizes,
122 subarray_starts, MPI_ORDER_C, MPI_DOUBLE, &depth_type);
123 } else {
124 int hsubarray_starts[] = {nhalo,nhalo,0};
125 int hsubarray_sizes[] = {ksize, jsize, nhalo};
126 MPI_Type_create_subarray(3, array_sizes, hsubarray_sizes,
127 hsubarray_starts, MPI_ORDER_C, MPI_DOUBLE, &horiz_type);
128
129 int vsubarray_starts[] = {nhalo, 0, nhalo};
130 int vsubarray_sizes[] = {ksize, nhalo, isize};
131 MPI_Type_create_subarray(3, array_sizes, vsubarray_sizes,
132 vsubarray_starts, MPI_ORDER_C, MPI_DOUBLE, &vert_type);
133
134 int dsubarray_starts[] = {0, nhalo, nhalo};
135 int dsubarray_sizes[] = {nhalo, ksize, isize};
136 MPI_Type_create_subarray(3, array_sizes, dsubarray_sizes,
137 dsubarray_starts, MPI_ORDER_C, MPI_DOUBLE, &depth_type);
138 }
139
140 MPI_Type_commit(&horiz_type);
141 MPI_Type_commit(&vert_type);
142 MPI_Type_commit(&depth_type);
```

Anotaciones del listado:

- Líneas 111–114 (y 124–127 en la rama sin esquinas): crea un tipo de datos horizontal usando `MPI_Type_create_subarray`.
- Líneas 116–118 (y 129–132): crea un tipo de datos vertical usando `MPI_Type_create_subarray`.
- Líneas 120–122 (y 134–137): crea un tipo de datos de profundidad (depth) usando `MPI_Type_create_subarray`.

El siguiente listado muestra que la rutina de comunicación que usa estos tres tipos de datos de MPI es bastante concisa.

**Listado 8.26 — La actualización de ghost cells 3D usando tipos de datos de MPI**

`GhostExchange/GhostExchange3D_VectorTypes/GhostExchange.cc`

```cpp
334 int waitcount = 12, ib1 = 4, ib2 = 8;
335 if (corners) {
336 waitcount=4;
337 ib1 = 0, ib2 = 0;
338 }
339
340 MPI_Request request[waitcount*nhalo];
341 MPI_Status status[waitcount*nhalo];
342
343 MPI_Irecv(&x[-nhalo][-nhalo][isize], 1, horiz_type, nrght, 1001,
344 MPI_COMM_WORLD, &request[0]);
345 MPI_Isend(&x[-nhalo][-nhalo][0], 1, horiz_type, nleft, 1001,
346 MPI_COMM_WORLD, &request[1]);
347
348 MPI_Irecv(&x[-nhalo][-nhalo][-nhalo], 1, horiz_type, nleft, 1002,
349 MPI_COMM_WORLD, &request[2]);
350 MPI_Isend(&x[-nhalo][-nhalo][isize-1], 1, horiz_type, nrght, 1002,
351 MPI_COMM_WORLD, &request[3]);
352 if (corners) MPI_Waitall(4, request, status);
353
354 MPI_Irecv(&x[-nhalo][jsize][-nhalo], 1, vert_type, ntop, 1003,
355 MPI_COMM_WORLD, &request[ib1+0]);
356 MPI_Isend(&x[-nhalo][0][-nhalo], 1, vert_type, nbot, 1003,
357 MPI_COMM_WORLD, &request[ib1+1]);
358
359 MPI_Irecv(&x[-nhalo][-nhalo][-nhalo], 1, vert_type, nbot, 1004,
360 MPI_COMM_WORLD, &request[ib1+2]);
361 MPI_Isend(&x[-nhalo][jsize-1][-nhalo], 1, vert_type, ntop, 1004,
362 MPI_COMM_WORLD, &request[ib1+3]);
363 if (corners) MPI_Waitall(4, request, status);
364
365 MPI_Irecv(&x[ksize][-nhalo][-nhalo], 1, depth_type, nback, 1005,
366 MPI_COMM_WORLD, &request[ib2+0]);
367 MPI_Isend(&x[0][-nhalo][-nhalo], 1, depth_type, nfrnt, 1005,
368 MPI_COMM_WORLD, &request[ib2+1]);
369
370 MPI_Irecv(&x[-nhalo][-nhalo][-nhalo], 1, depth_type, nfrnt, 1006,
371 MPI_COMM_WORLD, &request[ib2+2]);
372 MPI_Isend(&x[ksize-1][-nhalo][-nhalo], 1, depth_type, nback, 1006,
373 MPI_COMM_WORLD, &request[ib2+3]);
374 MPI_Waitall(waitcount, request, status);
```

Anotaciones del listado:

- Líneas 343–351: actualización de ghost cells para la dirección horizontal.
- Línea 352 (`if (corners) MPI_Waitall`): sincroniza si se necesitan las esquinas en la actualización.
- Líneas 354–362: actualización de ghost cells para la dirección vertical.
- Líneas 365–373: actualización de ghost cells para la dirección de profundidad (depth).

### 8.5.2 Soporte de topología cartesiana en MPI

En esta sección, le mostraremos cómo funcionan las funciones de topología en MPI. La operación sigue siendo el ghost exchange mostrado en la figura 8.5, pero podemos simplificar la codificación usando funciones cartesianas. No se cubren las funciones generales de grafos para aplicaciones no estructuradas. Comenzaremos con las rutinas de configuración antes de pasar a las rutinas de comunicación.

Las rutinas de configuración necesitan establecer los valores para las asignaciones de la cuadrícula de procesos y luego establecer los vecinos, como se hizo en los listados 8.18 y 8.22. Como se muestra en el listado 8.24 para 2D y en el listado 8.25 para 3D, el proceso establece el arreglo `dims` con el número de procesadores a usar en cada dimensión. Si alguno de los valores del arreglo `dims` es cero, la función `MPI_Dims_create` calcula algunos valores que funcionarán. Tenga en cuenta que el número de procesos en cada dirección no tiene en cuenta el tamaño de la malla y puede no producir buenos valores para problemas largos y estrechos. Considere el caso de una malla de 8x8x1000 y asígnele 8 procesadores; la cuadrícula de procesos será de 2x2x2, resultando en un dominio de malla de 4x4x500 en cada proceso.

`MPI_Cart_create` toma el arreglo `dims` resultante y un arreglo de entrada, `periodic`, que declara si una frontera se envuelve hacia el lado opuesto y viceversa. El último argumento es el argumento `reorder`, que permite a MPI reordenar los procesos. Es cero (falso) en este ejemplo. Ahora tenemos un nuevo comunicador que contiene información sobre la topología.

Obtener la disposición de la cuadrícula de procesos es simplemente una llamada a `MPI_Cart_coords`. Obtener los vecinos se hace con una llamada a `MPI_Cart_shift`, con el segundo argumento especificando la dirección y el tercer argumento el desplazamiento o número de procesos en esa dirección. La salida son los ranks de los procesadores adyacentes.

**Listado 8.27 — Soporte de topología cartesiana 2D en MPI**

`GhostExchange/CartExchange_Neighbor/CartExchange.cc`

```cpp
43 int dims[2] = {nprocy, nprocx};
44 int periodic[2]={0,0};
45 int coords[2];
46 MPI_Dims_create(nprocs, 2, dims);
47 MPI_Comm cart_comm;
48 MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periodic, 0, &cart_comm);
49 MPI_Cart_coords(cart_comm, rank, 2, coords);
50
51 int nleft, nrght, nbot, ntop;
52 MPI_Cart_shift(cart_comm, 1, 1, &nleft, &nrght);
53 MPI_Cart_shift(cart_comm, 0, 1, &nbot, &ntop);
```

La configuración de la topología cartesiana 3D es similar, pero con tres dimensiones, como muestra el siguiente listado.

**Listado 8.28 — Soporte de topología cartesiana 3D en MPI**

`GhostExchange/CartExchange3D_Neighbor/CartExchange.cc`

```cpp
65 int dims[3] = {nprocz, nprocy, nprocx};
66 int periods[3]={0,0,0};
67 int coords[3];
68 MPI_Dims_create(nprocs, 3, dims);
69 MPI_Comm cart_comm;
70 MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, 0, &cart_comm);
71 MPI_Cart_coords(cart_comm, rank, 3, coords);
72 int xcoord = coords[2];
73 int ycoord = coords[1];
74 int zcoord = coords[0];
75
76 int nleft, nrght, nbot, ntop, nfrnt, nback;
77 MPI_Cart_shift(cart_comm, 2, 1, &nleft, &nrght);
78 MPI_Cart_shift(cart_comm, 1, 1, &nbot, &ntop);
79 MPI_Cart_shift(cart_comm, 0, 1, &nfrnt, &nback);
```

Si comparamos este código con las versiones de los listados 8.19 y 8.23, vemos que las funciones de topología no ahorran muchas líneas de código ni reducen en gran medida la complejidad de programación en este ejemplo relativamente simple para la configuración. También podemos aprovechar el comunicador cartesiano creado en la línea 70 del listado 8.28 para realizar también la comunicación con los vecinos. Ahí es donde se ve la mayor reducción de líneas de código. La función de MPI tiene los siguientes argumentos:

```c
int MPI_Neighbor_alltoallw(const void *sendbuf,
 const int sendcounts[],
 const MPI_Aint sdispls[],
 const MPI_Datatype sendtypes[],
 void *recvbuf,
 const int recvcounts[],
 const MPI_Aint rdispls[],
 const MPI_Datatype recvtypes[],
 MPI_Comm comm)
```

Hay muchos argumentos en la llamada neighbor, pero una vez que los configuramos todos, la comunicación es concisa y se realiza en una sola sentencia. Repasaremos todos los argumentos en detalle, porque pueden ser difíciles de configurar correctamente.

La llamada de comunicación con vecinos puede usar un buffer ya lleno para los envíos y recepciones, o realizar la operación in situ (in place). Mostraremos el método in situ. Los buffers de envío y recepción son el arreglo 2D `x`. Usaremos un tipo de datos de MPI para describir el bloque de datos, así que los conteos (counts) serán un arreglo con el valor uno para los cuatro lados cartesianos en 2D o los seis lados en 3D. El orden de la comunicación para los lados es inferior, superior, izquierda, derecha en 2D, y frente, atrás, inferior, superior, izquierda, derecha en 3D, y es el mismo tanto para los tipos de envío como de recepción.

El bloque de datos es diferente para cada dirección: horizontal, vertical y profundidad (depth). Usamos la convención de los dibujos en perspectiva estándar, con x yendo hacia la derecha, y hacia arriba, y z (la profundidad) yendo hacia atrás dentro de la página. Pero dentro de cada dirección, el bloque de datos es el mismo, pero con diferentes desplazamientos hacia el inicio del bloque de datos. Los desplazamientos están en bytes, razón por la cual verá los offsets multiplicados por 8, el tamaño del tipo de datos de un valor de doble precisión. Ahora veamos cómo todo esto se plasma en código para la configuración de la comunicación para el caso 2D en el siguiente listado.

**Listado 8.29 — Configuración de la comunicación con vecinos cartesianos 2D**

`GhostExchange/CartExchange_Neighbor/CartExchange.c`

```c
55 int ibegin = imax *(coords[1] )/dims[1];
56 int iend = imax *(coords[1]+1)/dims[1];
57 int isize = iend - ibegin;
58 int jbegin = jmax *(coords[0] )/dims[0];
59 int jend = jmax *(coords[0]+1)/dims[0];
60 int jsize = jend - jbegin;
61
62 int jlow=nhalo, jhgh=jsize+nhalo, ilow=nhalo, inum = isize;
63 if (corners) {
64 int ilow = 0, inum = isize+2*nhalo;
65 if (nbot == MPI_PROC_NULL) jlow = 0;
66 if (ntop == MPI_PROC_NULL) jhgh = jsize+2*nhalo;
67 }
68 int jnum = jhgh-jlow;
69
70 int array_sizes[] = {jsize+2*nhalo, isize+2*nhalo};
71
72 int subarray_sizes_x[] = {jnum, nhalo};
73 int subarray_horiz_start[] = {jlow, 0};
74 MPI_Datatype horiz_type;
75 MPI_Type_create_subarray (2, array_sizes, subarray_sizes_x, subarray_horiz_start,
76 MPI_ORDER_C, MPI_DOUBLE, &horiz_type);
77 MPI_Type_commit(&horiz_type);
78
79 int subarray_sizes_y[] = {nhalo, inum};
80 int subarray_vert_start[] = {0, jlow};
81 MPI_Datatype vert_type;
82 MPI_Type_create_subarray (2, array_sizes, subarray_sizes_y, subarray_vert_start,
83 MPI_ORDER_C, MPI_DOUBLE, &vert_type);
84 MPI_Type_commit(&vert_type);
85
86 MPI_Aint sdispls[4] = { nhalo *(isize+2*nhalo)*8,
87                         jsize *(isize+2*nhalo)*8,
88                         nhalo *8,
89                         isize *8};
90 MPI_Aint rdispls[4] = { 0,
91                         (jsize+nhalo) *(isize+2*nhalo)*8,
92                         0,
93                         (isize+nhalo)*8};
94 MPI_Datatype sendtypes[4] = {vert_type, vert_type, horiz_type, horiz_type};
95 MPI_Datatype recvtypes[4] = {vert_type, vert_type, horiz_type, horiz_type};
```

Anotaciones del listado:

- Líneas 55–60: calcula los índices global de inicio y fin y el tamaño del arreglo local.
- Líneas 62–67: incluye los valores de las esquinas si se solicitan.
- Líneas 72–77: crea el bloque de datos a comunicar en la dirección horizontal usando la función subarray.
- Líneas 79–84: crea el bloque de datos a comunicar en la dirección vertical usando la función subarray.
- `sdispls` (líneas 86–89): los desplazamientos se miden en bytes desde la esquina inferior izquierda del bloque de memoria. Fila inferior: `nhalo` por encima del inicio (86); fila superior: `jsize` por encima del inicio (87); columna izquierda: `nhalo` a la derecha del inicio (88); columna derecha: `isize` a la derecha del inicio (89).
- `rdispls` (líneas 90–93): fila fantasma inferior: 0 por encima del inicio (90); fila fantasma superior: `jsize+nhalo` por encima del inicio (91); columna fantasma izquierda: 0 a la derecha del inicio (92); fila fantasma derecha: `jsize+nhalo` a la derecha del inicio (93).
- Línea 94 (`sendtypes`): los tipos de envío se ordenan como vecinos inferior, superior, izquierdo y derecho.
- Línea 95 (`recvtypes`): los tipos de recepción se ordenan como vecinos inferior, superior, izquierdo y derecho.

La configuración para la comunicación con vecinos cartesianos 3D usa los tipos de datos de MPI del listado 8.25. Los tipos de datos definen el bloque de datos a mover, pero necesitamos definir el offset en bytes hasta la ubicación de inicio del bloque de datos para el envío y la recepción. También necesitamos definir los arreglos para `sendtypes` y `recvtypes` en el orden apropiado, como en el siguiente listado.

**Listado 8.30 — Configuración de la comunicación con vecinos cartesianos 3D**

`GhostExchange/CartExchange3D_Neighbor/CartExchange.c`

```c
154 int xyplane_mult = (jsize+2*nhalo)*(isize+2*nhalo)*8;
155 int xstride_mult = (isize+2*nhalo)*8;
156 MPI_Aint sdispls[6] = { nhalo *xyplane_mult,
157                         ksize *xyplane_mult,
158                         nhalo *xstride_mult,
159                         jsize *xstride_mult,
160                         nhalo *8,
161                         isize *8};
162 MPI_Aint rdispls[6] = { 0,
163                         (ksize+nhalo) *xyplane_mult,
164                         0,
165                         (jsize+nhalo) *xstride_mult,
166                         0,
167                         (isize+nhalo)*8};
168 MPI_Datatype sendtypes[6] = { depth_type, depth_type, vert_type, vert_type, horiz_type, horiz_type};
169 MPI_Datatype recvtypes[6] = { depth_type, depth_type, vert_type, vert_type, horiz_type, horiz_type};
```

Anotaciones del listado:

- `sdispls` (líneas 156–161): los desplazamientos se miden en bytes desde la esquina inferior izquierda del bloque de memoria. Frente (front): `nhalo` detrás del frente (156); atrás (back): `ksize` detrás del frente (157); fila inferior: `nhalo` por encima del inicio (158); fila superior: `jsize` por encima del inicio (159); columna izquierda: `nhalo` a la derecha del inicio (160); columna derecha: `isize` (161).
- `rdispls` (líneas 162–167): fantasma del frente: 0 desde el frente (162); fantasma de atrás: `ksize+nhalo` detrás del frente (163); fila fantasma inferior: 0 por encima del inicio (164); fila fantasma superior: `jsize+nhalo` por encima del inicio (165); columna fantasma izquierda: 0 a la derecha del inicio (166); fila fantasma derecha: `jsize+nhalo` a la derecha del inicio (167).
- Líneas 168–169 (`sendtypes`, `recvtypes`): los tipos de envío y recepción se ordenan como frente, atrás, inferior, superior, izquierda y derecha.

La comunicación propiamente dicha se realiza con una única llamada a `MPI_Neighbor_alltoallw`, como se muestra en el listado 8.31. También hay un segundo bloque de código para los casos con esquinas que requiere un par de llamadas con una sincronización entre ellas para asegurar que las esquinas se llenen correctamente. La primera llamada hace solo la dirección horizontal y luego espera a que se complete antes de hacer la dirección vertical.

**Listado 8.31 — Comunicación con vecinos cartesianos 2D**

`GhostExchange/CartExchange_Neighbor/CartExchange.c`

```c
224 if (corners) {
225 int counts1[4] = {0, 0, 1, 1};
226 MPI_Neighbor_alltoallw ( &x[-nhalo][-nhalo], counts1, sdispls, sendtypes,
227 &x[-nhalo][-nhalo], counts1, rdispls, recvtypes,
228 cart_comm);
229
230 int counts2[4] = {1, 1, 0, 0};
231 MPI_Neighbor_alltoallw ( &x[-nhalo][-nhalo], counts2, sdispls, sendtypes,
232 &x[-nhalo][-nhalo], counts2, rdispls, recvtypes,
233 cart_comm);
234 } else {
235 int counts[4] = {1, 1, 1, 1};
236 MPI_Neighbor_alltoallw ( &x[-nhalo][-nhalo], counts, sdispls, sendtypes,
237 &x[-nhalo][-nhalo], counts, rdispls, recvtypes,
238 cart_comm);
239 }
```

Anotaciones del listado:

- Línea 225 (`counts1`): establece los conteos en 1 para la dirección horizontal.
- Líneas 226–228: comunicación horizontal.
- Línea 230 (`counts2`): establece los conteos en 1 para la dirección vertical.
- Líneas 231–233: comunicación vertical.
- Línea 235 (`counts`): establece todos los conteos en 1 para todas las direcciones.
- Líneas 236–238: toda la comunicación con los vecinos se realiza en una sola llamada.

La comunicación con vecinos cartesianos 3D es similar, pero con la adición de la coordenada z (profundidad). La profundidad va primero en los arreglos de conteos (counts) y tipos. En la comunicación por fases para las esquinas, la profundidad va después de los intercambios de ghost cells horizontal y vertical, como muestra el siguiente listado.

**Listado 8.32 — Comunicación con vecinos cartesianos 3D**

`GhostExchange/CartExchange3D_Neighbor/CartExchange.c`

```c
346 if (corners) {
347 int counts1[6] = {0, 0, 0, 0, 1, 1};
348 MPI_Neighbor_alltoallw( &x[-nhalo][-nhalo][-nhalo], counts1, sdispls, sendtypes,
349 &x[-nhalo][-nhalo][-nhalo], counts1, rdispls, recvtypes,
350 cart_comm);
351
352 int counts2[6] = {0, 0, 1, 1, 0, 0};
353 MPI_Neighbor_alltoallw( &x[-nhalo][-nhalo][-nhalo], counts2, sdispls, sendtypes,
354 &x[-nhalo][-nhalo][-nhalo], counts2, rdispls, recvtypes,
355 cart_comm);
356
357 int counts3[6] = {1, 1, 0, 0, 0, 0};
358 MPI_Neighbor_alltoallw( &x[-nhalo][-nhalo][-nhalo], counts3, sdispls, sendtypes,
359 &x[-nhalo][-nhalo][-nhalo], counts3, rdispls, recvtypes,
360 cart_comm);
361 } else {
362 int counts[6] = {1, 1, 1, 1, 1, 1};
363 MPI_Neighbor_alltoallw( &x[-nhalo][-nhalo][-nhalo], counts, sdispls, sendtypes,
364 &x[-nhalo][-nhalo][-nhalo], counts, rdispls, recvtypes,
365 cart_comm);
366 }
```

Anotaciones del listado:

- Líneas 347–350 (`counts1`): intercambio de ghost cells horizontal.
- Líneas 352–355 (`counts2`): intercambio de ghost cells vertical.
- Líneas 357–360 (`counts3`): intercambio de ghost cells de profundidad (depth).
- Líneas 362–365 (rama `else`): todos los vecinos a la vez.

### 8.5.3 Pruebas de rendimiento de las variantes de intercambio de ghost cells

Probemos estas variantes de intercambio de ghost cells en un sistema de prueba. Usaremos dos nodos Broadwell (CPU Intel® Xeon® E5-2695 v4 a 2.10 GHz) con 72 núcleos virtuales cada uno. Podríamos ejecutar esto en más nodos de cómputo con diferentes implementaciones de bibliotecas MPI, tamaños de halo, tamaños de malla y con interconexiones de comunicación de mayor rendimiento, para una visión más completa de cómo se desempeña cada variante de intercambio de ghost cells. Aquí está el código:

```bash
mpirun -n 144 --bind-to hwthread ./GhostExchange -x 12 -y 12 -i 20000 \
 -j 20000 -h 2 -t -c
mpirun -n 144 --bind-to hwthread ./GhostExchange -x 6 -y 4 -z 6 -i 700 \
 -j 700 -k 700 -h 2 -t -c
```

Las opciones del programa GhostExchange son:

- `-x` (procesos en la dirección x)
- `-y` (procesos en la dirección y)
- `-z` (procesos en la dirección z)
- `-i` (tamaño de malla en la dirección i o x)
- `-j` (tamaño de malla en la dirección j o y)
- `-k` (tamaño de malla en la dirección k o z)
- `-h` (ancho de las celdas de halo, usualmente 1 o 2)
- `-c` (incluir las celdas de las esquinas)

> **Ejercicio: Pruebas de ghost cells**
>
> El código fuente que acompaña al capítulo está configurado para ejecutar todo el conjunto de variaciones de intercambio de ghost cells. En el archivo `batch.sh` puede cambiar el tamaño del halo y si desea esquinas. El archivo está configurado para ejecutar todos los casos de prueba 11 veces en dos nodos Skylake Gold con 144 procesos en total.
>
> ```bash
> cd GhostExchange
> ./build.sh
> ./batch.sh |& tee results.txt
> ./get_stats.sh > stats.out
> ```
>
> Luego puede generar gráficas con los scripts proporcionados. Necesita la biblioteca matplotlib para los scripts de graficación de Python. Los resultados son para el tiempo de ejecución mediano.
>
> ```bash
> python plottimebytype.py
> python plottimeby3Dtype.py
> ```

La siguiente figura muestra las gráficas para los casos de prueba pequeños. Las versiones con tipos de datos de MPI parecen un poco más rápidas incluso a esta escala pequeña, lo que indica que quizás se esté evitando una copia de datos. La mayor ganancia de las llamadas de topología cartesiana de MPI y los tipos de datos de MPI es que el código del ghost exchange se simplifica enormemente. El uso de estas llamadas de MPI más avanzadas sí requiere más esfuerzo de configuración, pero esto se hace solo una vez al inicio.

> **Figura (gráficas de rendimiento)** El rendimiento relativo de los intercambios de ghost cells 2D y 3D en dos nodos con un total de 144 procesos. Los tipos de datos de MPI en las variantes *MPI types* y *CNeighbor* son un poco más rápidos que el buffer llenado explícitamente mediante bucles de asignaciones de arreglo. En los intercambios de ghost cells 2D, las rutinas de pack son más lentas, aunque en este caso el buffer llenado explícitamente es más rápido que los tipos de datos de MPI.

Contenido de la figura:

- **Tiempo de ejecución del ghost exchange 2D (s)** — métodos de comunicación con vecinos en 2D: `Array`, `Pack`, `MPI types`, `CNeighbor`, `CMPI types`, `CPack`.
- **Tiempo de ejecución del ghost exchange 3D (s)** — métodos de comunicación con vecinos en 3D: `Array`, `MPI types`, `CArray`, `CNeighbor`, `CMPI types`.


## 8.6 MPI híbrido más OpenMP para escalabilidad extrema

La combinación de dos o más técnicas de paralelización se denomina paralelización híbrida (hybrid parallelization), en contraste con todas las implementaciones de MPI, que también se denominan MPI puro (pure MPI) o MPI-en-todas-partes (MPI-everywhere). En esta sección, veremos el MPI híbrido más OpenMP, donde MPI y OpenMP se usan juntos en una aplicación. Esto generalmente equivale a reemplazar algunos ranks de MPI con hilos (threads) de OpenMP. Para aplicaciones paralelas más grandes que alcanzan miles de procesos, reemplazar ranks de MPI con hilos de OpenMP reduce potencialmente el tamaño total del dominio de MPI y la memoria necesaria para la escala extrema. Sin embargo, el rendimiento añadido de la capa de paralelismo a nivel de hilos podría no compensar siempre la complejidad y el tiempo de desarrollo adicionales. Por esta razón, las implementaciones de MPI híbrido más OpenMP son normalmente el dominio de aplicaciones extremas, tanto en tamaño como en necesidades de rendimiento.

### 8.6.1 Los beneficios del MPI híbrido más OpenMP

Cuando el rendimiento se vuelve lo suficientemente crítico como para justificar la complejidad adicional del paralelismo híbrido, puede haber varias ventajas en añadir una capa paralela de OpenMP al código basado en MPI. Por ejemplo, estas ventajas podrían ser:

- Menos ghost cells que comunicar entre nodos
- Menores requisitos de memoria para los buffers de MPI
- Menor contención por la NIC
- Tamaño reducido de las comunicaciones basadas en árboles
- Mejor balanceo de carga
- Acceso a todos los componentes de hardware

Las aplicaciones paralelas descompuestas espacialmente que usan subdominios con ghost cells (celdas de halo) tendrán menos ghost cells totales por nodo cuando añada paralelismo a nivel de hilos. Esto conduce a una reducción tanto en los requisitos de memoria como en los costos de comunicación, especialmente en una arquitectura de muchos núcleos (many-core) como la Knights Landing (KNL) de Intel. Usar paralelismo de memoria compartida también puede mejorar el rendimiento al reducir la contención por la tarjeta de interfaz de red (NIC), evitando la copia innecesaria de datos que usa MPI para los mensajes dentro del nodo. Además, muchos algoritmos de MPI están basados en árboles, escalando como log2n. Reducir el tiempo de ejecución en 2n hilos disminuye la profundidad del árbol y mejora incrementalmente el rendimiento. Aunque el trabajo restante todavía tiene que ser realizado por los hilos, impacta el rendimiento al permitir menores costos de latencia de sincronización y comunicación. Los hilos también pueden usarse para mejorar el balanceo de carga dentro de una región NUMA o un nodo de cómputo.

En algunos casos, un enfoque paralelo híbrido no solo es ventajoso, sino necesario para acceder al potencial completo de rendimiento del hardware. Por ejemplo, algún hardware, y quizás la funcionalidad del controlador de memoria, solo puede ser accedido por hilos y no por procesos (ranks de MPI). Las arquitecturas de muchos núcleos Knights Corner y Knights Landing de Intel han tenido estas preocupaciones. En MPI + X + Y, donde X es el threading e Y es un lenguaje de GPU, a menudo emparejamos los ranks con el número de GPUs. OpenMP permite que la aplicación siga accediendo a los demás procesadores para el trabajo en la CPU. Hay otras soluciones para esto, como los grupos MPI_COMM y la funcionalidad de memoria compartida de MPI, o simplemente controlar la GPU desde múltiples ranks de MPI.

En resumen, aunque puede ser atractivo ejecutar códigos con MPI-en-todas-partes (MPI-everywhere) en sistemas modernos de muchos núcleos, existen preocupaciones sobre la escalabilidad a medida que crece el número de núcleos. Si busca escalabilidad extrema, querrá una implementación eficiente de OpenMP en su aplicación. Cubrimos nuestro diseño de OpenMP de alto nivel, que es mucho más eficiente, en el capítulo anterior, en las secciones 7.2.2 y 7.6.

### 8.6.2 Ejemplo de MPI más OpenMP

El primer paso para una implementación de MPI híbrido más OpenMP es informar a MPI de lo que va a hacer. Esto se hace en la llamada `MPI_Init`, justo al inicio del programa. Debe reemplazar la llamada `MPI_Init` con la llamada `MPI_Init_thread`, así:

```c
MPI_Init_thread(&argc, &argv, int thread_model required,
 int *thread_model_provided);
```

El estándar MPI define cuatro modelos de hilos (thread models). Estos modelos proporcionan diferentes niveles de seguridad de hilos (thread safety) con las llamadas de MPI. En orden creciente de seguridad de hilos:

- `MPI_THREAD_SINGLE` — Solo se ejecuta un hilo (MPI estándar)
- `MPI_THREAD_FUNNELED` — Multihilo, pero solo el hilo principal hace llamadas a MPI
- `MPI_THREAD_SERIALIZED` — Multihilo, pero solo un hilo a la vez hace llamadas a MPI
- `MPI_THREAD_MULTIPLE` — Multihilo, con múltiples hilos haciendo llamadas a MPI

Muchas aplicaciones realizan la comunicación al nivel del bucle principal, y los hilos de OpenMP se aplican a los bucles computacionales clave. Para este patrón, `MPI_THREAD_FUNNELED` funciona perfectamente.

> **NOTA** Es mejor usar el nivel más bajo de seguridad de hilos que necesite. Cada nivel superior impone una penalización de rendimiento, porque la biblioteca de MPI tiene que colocar mutexes o bloques críticos alrededor de las colas de envío y recepción y de otras partes básicas de MPI.

Ahora veamos qué cambios se necesitan en nuestro ejemplo de stencil para añadir el threading de OpenMP. Elegimos el ejemplo CartExchange_Neighbor para modificar en este ejercicio. El siguiente listado muestra que el primer cambio es modificar la inicialización de MPI.

**Listado 8.33 — Inicialización de MPI para el threading de OpenMP**

`HybridMPIPlusOpenMP/CartExchange.cc`

```cpp
26 int provided;
27 MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
28
29 int rank, nprocs;
30 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
31 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
32 if (rank == 0) {
33 #pragma omp parallel
34 #pragma omp master
35 printf("requesting MPI_THREAD_FUNNELED" " with %d threads\n",
36 omp_get_num_threads());
37 if (provided != MPI_THREAD_FUNNELED){
38 printf("Error: MPI_THREAD_FUNNELED" " not available. Aborting ...\n");
39 MPI_Finalize();
40 exit(0);
41 }
42 }
```

Anotaciones del listado:

- Línea 27 (`MPI_Init_thread`): inicialización de MPI para el threading de OpenMP.
- Líneas 35–36 (`printf` con `omp_get_num_threads`): imprime el número de hilos para verificar si es el que queremos.
- Línea 37 (comprobación de `provided`): comprueba si este MPI soporta el nivel de seguridad de hilos que solicitamos.

El cambio obligatorio es usar `MPI_Init_thread` en lugar de `MPI_Init` en la línea 27. El código adicional comprueba que el nivel de seguridad de hilos solicitado está disponible y sale si no lo está. También imprimimos el número de hilos en el hilo principal del rank cero. Ahora pasemos a los cambios en el bucle computacional que se muestra en el siguiente listado.

**Listado 8.34 — Adición del threading de OpenMP y la vectorización a los bucles computacionales**

`HybridMPIPlusOpenMP/CartExchange.cc`

```cpp
157 #pragma omp parallel for
158 for (int j = 0; j < jsize; j++){
159 #pragma omp simd
160 for (int i = 0; i < isize; i++){
161 xnew[j][i] = ( x[j][i] + x[j][i-1] + x[j][i+1] + x[j-1][i] + x[j+1][i] )/5.0;
162 }
163 }
```

Anotaciones del listado:

- Línea 157 (`#pragma omp parallel for`): añade el threading de OpenMP para el bucle externo.
- Línea 159 (`#pragma omp simd`): añade la vectorización SIMD para el bucle interno.

Los cambios necesarios para añadir el threading de OpenMP son la adición de un único pragma en la línea 157. Como extra, mostramos cómo añadir vectorización para el bucle interno con otro pragma insertado en la línea 159.

Ahora puede intentar ejecutar este ejemplo de MPI híbrido más OpenMP + Vectorización en su sistema. Pero para obtener un buen rendimiento, necesitará controlar la ubicación (placement) de los ranks de MPI y los hilos de OpenMP. Esto se hace estableciendo la afinidad (affinity), un tema que cubriremos con mayor profundidad en el capítulo 14.

> **DEFINICIÓN** La afinidad (affinity) asigna una preferencia para la planificación (scheduling) de un proceso, rank o hilo hacia un componente de hardware en particular. Esto también se denomina anclaje (pinning) o vinculación (binding).

Establecer la afinidad para sus ranks e hilos se vuelve más importante a medida que aumenta la complejidad del nodo y con las aplicaciones paralelas híbridas. En ejemplos anteriores, usamos `--bind-to core` y `--bind-to hwthread` para mejorar el rendimiento y reducir la variabilidad en el rendimiento en tiempo de ejecución causada por la migración de ranks de un núcleo a otro. En OpenMP, usamos variables de entorno para establecer la ubicación y las afinidades. Un ejemplo es:

```bash
export OMP_PLACES=cores
export OMP_CPU_BIND=true
```

Por ahora, comience anclando los ranks de MPI a los sockets, de modo que los hilos puedan distribuirse a otros núcleos, como mostramos en nuestro ejemplo de prueba de ghost cells para el procesador Skylake Gold. Así es cómo:

```bash
export OMP_NUM_THREADS=22
mpirun -n 4 --bind-to socket ./CartExchange -x 2 -y 2 -i 20000 -j 20000 \
-h 2 -t -c
```

Ejecutamos 4 ranks de MPI, cada uno de los cuales genera 22 hilos, según lo especificado por la variable de entorno `OMP_NUM_THREADS`, para un total de 88 procesos. La opción `--bind-to socket` de `mpirun` le indica que vincule los procesos al socket donde están ubicados.

## 8.7 Exploraciones adicionales

Aunque hemos cubierto mucho material en este capítulo, todavía hay muchas más características que vale la pena explorar a medida que gana más experiencia con MPI. Algunas de las más importantes se mencionan aquí y se dejan para su propio estudio.

- **Grupos de comunicadores (Comm groups)** — MPI tiene un rico conjunto de funciones que crean, dividen y manipulan de otras maneras el comunicador estándar MPI_COMM_WORLD en nuevas agrupaciones para operaciones especializadas, como la comunicación dentro de una fila o subgrupos basados en tareas. Para algunos ejemplos del uso de grupos de comunicadores, vea el listado 16.4 en la sección 16.3. Usamos grupos de comunicación para dividir la salida de archivos en múltiples archivos y dividir el dominio en comunicadores de fila y columna.
- **Comunicaciones de frontera de mallas no estructuradas (Unstructured mesh boundary communications)** — Una malla no estructurada necesita intercambiar datos de frontera de manera similar a la cubierta para una malla regular cartesiana. Estas operaciones son más complejas y no se cubren aquí. Hay muchas bibliotecas de comunicación dispersas (sparse) y basadas en grafos que dan soporte a las aplicaciones de mallas no estructuradas. Un ejemplo de tal biblioteca es la biblioteca de comunicación L7, desarrollada por Richard Barrett, ahora en los Sandia National Laboratories. Está incluida con la mini-app CLAMR; vea el subdirectorio `l7` en <https://github.com/LANL/CLAMR>.
- **Memoria compartida (Shared memory)** — Las implementaciones originales de MPI enviaban datos a través de la interfaz de red en casi todos los casos. A medida que el número de núcleos creció, los desarrolladores de MPI se dieron cuenta de que podían realizar parte de la comunicación en memoria compartida. Esto se hace tras bambalinas como una optimización de la comunicación. Se sigue añadiendo funcionalidad adicional de memoria compartida con las "ventanas" (windows) de memoria compartida de MPI. Esta funcionalidad tuvo algunos problemas al principio, pero se está volviendo lo suficientemente madura como para usarse en aplicaciones.
- **Comunicación unilateral (One-sided communication)** — Respondiendo a otros modelos de programación, MPI añadió la comunicación unilateral en la forma de `MPI_Put` y `MPI_Get`. Al contrario del modelo original de paso de mensajes de MPI, donde tanto el emisor como el receptor tienen que ser participantes activos, el modelo unilateral permite que solo uno u otro realice la operación.

### 8.7.1 Lecturas adicionales

Si desea más material introductorio sobre MPI, el texto de Peter Pacheco es un clásico:

Peter Pacheco, *An introduction to parallel programming* (Elsevier, 2011).

Puede encontrar una cobertura exhaustiva de MPI escrita por miembros del equipo de desarrollo original de MPI:

William Gropp, et al., *Using MPI: portable parallel programming with the message-passing interface*, Vol. 1 (MIT Press, 1999).

Para una presentación de MPI más OpenMP, hay una buena conferencia (lecture) de un curso de Bill Gropp, uno de los desarrolladores del estándar MPI original. Aquí está el enlace:

<http://wgropp.cs.illinois.edu/courses/cs598-s16/lectures/lecture36.pdf>

### 8.7.2 Ejercicios

1. ¿Por qué no podemos simplemente bloquear en las recepciones, como se hizo en el send/receive del ghost exchange usando los métodos de buffer con pack o con arreglo en los listados 8.20 y 8.21, respectivamente?
2. ¿Es seguro bloquear en las recepciones como se muestra en el listado 8.8 en la versión con tipo vector del ghost exchange? ¿Cuáles son las ventajas si solo bloqueamos en las recepciones?
3. Modifique el ejemplo del ghost exchange con tipo vector del listado 8.21 para usar recepciones bloqueantes (blocking) en lugar de un waitall. ¿Es más rápido? ¿Funciona siempre?
4. Intente reemplazar los tags explícitos en una de las rutinas de ghost exchange con `MPI_ANY_TAG`. ¿Funciona? ¿Es más rápido? ¿Qué ventaja ve en usar tags explícitos?
5. Elimine las barreras de los temporizadores sincronizados en uno de los ejemplos de ghost exchange. Ejecute el código con los temporizadores sincronizados originales y con los temporizadores no sincronizados.
6. Añada las estadísticas del temporizador del listado 8.11 al código de medición de ancho de banda del stream triad del listado 8.17.
7. Aplique los pasos para convertir el OpenMP de alto nivel al ejemplo de MPI híbrido más OpenMP en el código que acompaña al capítulo (directorio HybridMPIPlusOpenMP). Experimente con la vectorización, el número de hilos y los ranks de MPI en su plataforma.

## Resumen

- Use los mensajes punto a punto de send y receive apropiados. Esto evita los cuelgues (hangs) y obtiene un buen rendimiento.
- Use la comunicación colectiva para las operaciones comunes. Esto hace que la programación sea concisa, evita los cuelgues y mejora el rendimiento.
- Use los ghost exchanges para enlazar los subdominios de varios procesadores. Los intercambios hacen que los subdominios actúen como una única malla computacional global.
- Añada más niveles de paralelismo combinando MPI con hilos de OpenMP y vectorización. El paralelismo adicional ayuda a dar un mejor rendimiento.
