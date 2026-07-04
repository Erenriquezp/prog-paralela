# Ejemplos del Capítulo 8 (MPI) — versión para examen

Recreación simple de los ejemplos del capítulo 8 (`Practice/notes/Capitulo_8_MPI_completo_es.md`),
todo dentro de `main`, sin funciones auxiliares. Solo los que caben en un examen de 1 hora.

| Archivo | Listado del libro | Qué enseña |
|---|---|---|
| `01_minimo.cpp` | 8.1 | Esqueleto mínimo de MPI |
| `02_sendrecv.cpp` | 8.5–8.7 | Intercambio entre parejas SIN deadlock (3 formas seguras) |
| `03_barrera_wtime.cpp` | 8.9 | `MPI_Barrier` + `MPI_Wtime` para medir tiempos |
| `04_bcast_archivo.cpp` | 8.10 | `MPI_Bcast` en 2 pasos: tamaño y luego contenido |
| `05_reduce_min_max_avg.cpp` | 8.11 | `MPI_Reduce` con `MPI_MAX`, `MPI_MIN`, `MPI_SUM` |
| `06_gather_impresion.cpp` | 8.15 | `MPI_Gather` para imprimir ordenado desde rank 0 |
| `07_scatterv_gatherv.cpp` | 8.16 | Bloques VARIABLES: `Allgather` + `Scatterv` + `Gatherv` |
| `08_stream_triad.cpp` | 8.17 | Paralelismo de datos sin comunicación (ancho de banda) |

No se recrean (demasiado largos para 1 hora): suma de Kahan con operador propio (8.12–8.14),
ghost cells 2D/3D (8.18–8.26), topología cartesiana (8.27–8.29) e híbrido MPI+OpenMP.

## Ideas clave para el examen

- **Deadlock** (`02`): si todos hacen `Recv` primero, nadie envía → se cuelga.
  Soluciones: alternar por par/impar, `MPI_Sendrecv`, o `Isend`/`Irecv` + `MPI_Waitall`.
- **Medir tiempos** (`03`, `05`): barrera antes de arrancar y antes de parar el reloj;
  con `Reduce` se obtiene min/max/promedio entre ranks.
- **Bcast de datos de tamaño desconocido** (`04`): primero el tamaño, luego el contenido.
- **Reparto sin padding** (`07`, `08`): `inicio = N*rank/nprocs`, `fin = N*(rank+1)/nprocs`.
  Con bloques desiguales se usan `Scatterv`/`Gatherv` + arreglos de tamaños y offsets
  (alternativa al padding de `07.plantillas-mpi`).

## Compilar y ejecutar

```powershell
set PATH=c:/tools/mingw64/bin;%PATH%
cmake -B build -G "Ninja Multi-Config" `
  -DCMAKE_C_COMPILER=c:/tools/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=c:/tools/mingw64/bin/g++.exe `
  -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic
cmake --build build --config Debug

cd C:\oneAPI && setvars
# 04_bcast_archivo lee entrada.txt: ejecutarlo DESDE esta carpeta
cd C:\tools\prog-paralela\08.ejemplos-cap8-mpi
mpiexec -n 4 build\Debug\01_minimo.exe
```

`02_sendrecv` requiere un número PAR de procesos.
