# Plantillas MPI para estudio

Tres plantillas mínimas, todo dentro de `main`, para implementar cualquier programa MPI del curso.

| Archivo | Qué enseña |
|---|---|
| `01_base.cpp` | Esqueleto obligatorio: `Init`, `Comm_size`, `Comm_rank`, `Finalize` |
| `02_punto_a_punto.cpp` | Maestro-esclavo manual con `MPI_Send` / `MPI_Recv` |
| `03_colectivas.cpp` | Lo mismo pero con `Bcast`, `Scatter`, `Gather`, `Reduce` y padding |

## La receta (memorizar)

Todo programa MPI del curso tiene 5 fases:

1. **Esqueleto**: `MPI_Init` → `Comm_size` → `Comm_rank` ... `MPI_Finalize`.
2. **Reparto**: `bloque = ceil(N / nprocs)`; el sobrante es `padding = bloque * nprocs - N`.
   - Punto a punto: el último rank trabaja con `bloque - padding` elementos.
   - Colectivas: se agranda el arreglo global a `bloque * nprocs` relleno con 0.
3. **Distribución**: el rank 0 entrega datos (`Send`/`Recv` o `Bcast` + `Scatter`).
4. **Cálculo local**: cada rank procesa SOLO su bloque (aquí cambia cada problema).
5. **Recolección**: el rank 0 junta resultados (`Recv`/`Send` o `Gather` / `Reduce`).

## Firmas esenciales

```cpp
MPI_Send(buffer, cantidad, MPI_DOUBLE, destino, tag, MPI_COMM_WORLD);
MPI_Recv(buffer, cantidad, MPI_DOUBLE, origen,  tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

MPI_Bcast  (buffer, cantidad, MPI_DOUBLE, raiz, MPI_COMM_WORLD);
MPI_Scatter(origen, cant_por_rank, MPI_DOUBLE, destino, cant_por_rank, MPI_DOUBLE, raiz, MPI_COMM_WORLD);
MPI_Gather (origen, cant_por_rank, MPI_DOUBLE, destino, cant_por_rank, MPI_DOUBLE, raiz, MPI_COMM_WORLD);
MPI_Reduce (&local, &total, 1, MPI_DOUBLE, MPI_SUM, raiz, MPI_COMM_WORLD);
```

Reglas de oro:
- Las **colectivas las llaman TODOS los ranks** (sin `if (rank == 0)`), aunque el
  buffer global solo importe en la raíz.
- En punto a punto, usar **tags distintos** para distribución (0) y recolección (1).
- Imprimir solo hasta `N` para descartar el padding.

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
mpiexec -n 4 C:\tools\prog-paralela\07.plantillas-mpi\build\Debug\03_colectivas.exe
```
