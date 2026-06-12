cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

find_package(fmt CONFIG REQUIRED)

#-- config MPI
set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

add_executable(matrices_mult_colectiva
    matrices_mult_colectiva.cpp 
)

target_include_directories(matrices_mult_colectiva PRIVATE ${MPI_INCLUDE})

target_link_libraries(matrices_mult_colectiva PRIVATE 
    fmt::fmt
    ${MPI_LIB}/impi.lib
)