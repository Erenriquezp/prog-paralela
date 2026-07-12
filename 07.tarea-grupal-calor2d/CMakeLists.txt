cmake_minimum_required(VERSION 3.16)

project(Calor2D)

set(CMAKE_CXX_STANDARD 20)
# -mavx2: intrínsecas SIMD | -fopenmp: OpenMP | -Wall: la rúbrica exige cero warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -fopenmp -Wall")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

#-- config MPI (Intel oneAPI, mismo patrón que 05.ejemplo-mpi)
set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

add_executable(calor2d
    main.cpp
    args.cpp
    palette.cpp
    heat_serial.cpp
    heat_simd.cpp
    heat_openmp.cpp
    heat_mpi.cpp
)

target_include_directories(calor2d PRIVATE ${MPI_INCLUDE})

target_link_libraries(calor2d PRIVATE
    fmt::fmt
    SFML::System SFML::Graphics SFML::Window
    ${MPI_LIB}/impi.lib
)
