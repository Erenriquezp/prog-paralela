cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

find_package(fmt CONFIG REQUIRED)

#-- config MPI
set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

add_executable(ejemplo-mpi 
    matrices_mult_padding.cpp 
)

target_include_directories(ejemplo-mpi PRIVATE ${MPI_INCLUDE})

target_link_libraries(ejemplo-mpi PRIVATE 
    fmt::fmt
    ${MPI_LIB}/impi.lib
)

add_executable(matrices_mult
    matrices_mult.cpp
)

target_link_libraries(matrices_mult PRIVATE 
    fmt::fmt
    ${MPI_LIB}/impi.lib
)

target_include_directories(matrices_mult PRIVATE ${MPI_INCLUDE})
