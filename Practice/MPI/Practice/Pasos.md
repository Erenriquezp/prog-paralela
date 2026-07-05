### El Algoritmo Universal de MPI

| Fase | Paso | Descripción Técnica |
| --- | --- | --- |
| **Inicio** | 1. `Includes` | `iostream, vector, cmath, mpi, fmt`. |
|  | 2. `Init` | `Init` (arranque), `Size` (¿cuántos?), `Rank` (¿quién soy?). |
| **Matemática** | 3. `Padding` | `bloque = ceil(N/P)`, `total = bloque*P`. (El relleno es obligatorio para el chasis simétrico). |
| **Reserva** | 4. `Buffers` | **Esclavos:** Reserva solo `local` (del tamaño del bloque). **Maestro:** Reserva `Global` (del tamaño `total`) + `Local`. |
| **Distribución** | 5. `Envío` | `If rank 0`: `for(1..N)` $\rightarrow$ `MPI_Send`. **Else**: `MPI_Recv`. **¡No olvidar copiar mi parte en el Rank 0!** |
| **Cálculo** | 6. `Trabajo` | **Todos** procesan su `local` sin hablar con nadie. (Lógica matemática pura). |
| **Recolección** | 7. `Envío` | `If rank 0`: Copia su parte en `Global`, luego `for(1..N)` $\rightarrow$ `MPI_Recv`. **Else**: `MPI_Send`. |
| **Cierre** | 8. `Impresión` | Solo el `rank 0` imprime. |
|  | 9. `Finalize` | `MPI_Finalize`. |

---
