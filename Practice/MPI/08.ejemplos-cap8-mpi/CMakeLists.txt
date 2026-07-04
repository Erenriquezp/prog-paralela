cmake_minimum_required(VERSION 3.16)

project(EjemplosCap8MPI)

find_package(fmt CONFIG REQUIRED)

#-- config MPI
set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

set(EJEMPLOS
    01_minimo
    02_sendrecv
    03_barrera_wtime
    04_bcast_archivo
    05_reduce_min_max_avg
    06_gather_impresion
    07_scatterv_gatherv
    08_stream_triad
)

foreach(ejemplo ${EJEMPLOS})
    add_executable(${ejemplo} ${ejemplo}.cpp)
    target_include_directories(${ejemplo} PRIVATE ${MPI_INCLUDE})
    target_link_libraries(${ejemplo} PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)
endforeach()
