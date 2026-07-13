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

add_executable(05_suma 05_suma.cpp)
target_include_directories(05_suma PRIVATE ${MPI_INCLUDE})
target_link_libraries(05_suma PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(06_max 06_max.cpp)
target_include_directories(06_max PRIVATE ${MPI_INCLUDE})
target_link_libraries(06_max PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(07_exterior_colectivas 07_exterior_colectivas.cpp)
target_include_directories(07_exterior_colectivas PRIVATE ${MPI_INCLUDE})
target_link_libraries(07_exterior_colectivas PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(08_suma_colectivas 08_suma_colectivas.cpp)
target_include_directories(08_suma_colectivas PRIVATE ${MPI_INCLUDE})
target_link_libraries(08_suma_colectivas PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(09_leer_archivo 09_leer_archivo.cpp)
target_include_directories(09_leer_archivo PRIVATE ${MPI_INCLUDE})
target_link_libraries(09_leer_archivo PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(10_estadisticas 10_estadisticas.cpp)
target_include_directories(10_estadisticas PRIVATE ${MPI_INCLUDE})
target_link_libraries(10_estadisticas PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(11_vocales 11_vocales.cpp)
target_include_directories(11_vocales PRIVATE ${MPI_INCLUDE})
target_link_libraries(11_vocales PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)

add_executable(12_vocales_colectivas 12_vocales_colectivas.cpp)
target_include_directories(12_vocales_colectivas PRIVATE ${MPI_INCLUDE})
target_link_libraries(12_vocales_colectivas PRIVATE fmt::fmt ${MPI_LIB}/impi.lib)
