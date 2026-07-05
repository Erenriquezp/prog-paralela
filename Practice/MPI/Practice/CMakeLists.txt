cmake_minimum_required(VERSION 3.16)

project(PracticeMPI)

find_package(fmt CONFIG REQUIRED)

set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

add_executable(01_base 01_base.cpp)
target_include_directories(01_base PRIVATE ${MPI_INCLUDE})
target_link_libraries(01_base PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(02_punto_a_punto 02_punto_a_punto.cpp)
target_include_directories(02_punto_a_punto PRIVATE ${MPI_INCLUDE})
target_link_libraries(02_punto_a_punto PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(03_colectivas 03_colectivas.cpp)
target_include_directories(03_colectivas PRIVATE ${MPI_INCLUDE})
target_link_libraries(03_colectivas PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(04_producto_exterior 04_producto_exterior.cpp)
target_include_directories(04_producto_exterior PRIVATE ${MPI_INCLUDE})
target_link_libraries(04_producto_exterior PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)