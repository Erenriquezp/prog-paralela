cmake_minimum_required(VERSION 3.16)

project(PracticaHibrida)

enable_language(CXX CUDA)

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)
find_package(OpenMP REQUIRED)

#-- config MPI
set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

include_directories(
    ${MPI_INCLUDE}
)

add_executable(practica-hibrida 
    main.cpp 
    palette.cpp
    fractal.cpp
    kernel.cu
)

target_include_directories(practica-hibrida PRIVATE 
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    ${MPI_INCLUDE}
)

target_link_libraries(practica-hibrida PRIVATE 
    fmt::fmt 
    SFML::System SFML::Graphics SFML::Window 
    OpenMP::OpenMP_CXX
    ${MPI_LIB}/impi.lib
)
