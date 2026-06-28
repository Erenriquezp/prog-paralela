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
