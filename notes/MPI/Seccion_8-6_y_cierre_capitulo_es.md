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
