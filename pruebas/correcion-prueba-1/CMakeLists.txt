cmake_minimum_required(VERSION 3.16)

project(Prueba)

#-- config MPI
set(MPI_ROOT "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -fopenmp")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)
find_package(Freetype REQUIRED)

include_directories(
    ${MPI_INCLUDE}
)

add_executable(prueba 
    main.cpp 
    palette.cpp
    fractal.cpp
    draw_text.cpp
)

target_include_directories(prueba PRIVATE ${MPI_INCLUDE})

target_link_libraries(prueba PRIVATE fmt::fmt 
    SFML::System SFML::Graphics SFML::Window 
    Freetype::Freetype
    ${MPI_LIB}/impi.lib)
