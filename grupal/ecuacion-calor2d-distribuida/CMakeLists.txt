cmake_minimum_required(VERSION 3.16)

project(ejemplo_calor_mpi)
 

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS 
                    Network
                    System 
                    Graphics 
                    Window
                    Audio
                    CONFIG REQUIRED)
find_package(Freetype REQUIRED)

set(MPI_ROOT "C:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

include_directories(
    ${MPI_INCLUDE}
)

    add_executable(calor-mpi
    main.cpp
    calor_mpi.cpp
    palette.cpp
)

target_link_libraries(calor-mpi PRIVATE 
    fmt::fmt 
    SFML::System
    SFML::Graphics
    SFML::Window
    SFML::Network
    SFML::Audio
    Freetype::Freetype
    ${MPI_LIB}/impi.lib)


