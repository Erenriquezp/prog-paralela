cmake_minimum_required(VERSION 3.16)

project(PracticaFinal)

enable_language(CXX CUDA)

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(practica-final 
    main.cpp
    kernel.cu
    palette.cpp
)

target_include_directories(practica-final PRIVATE 
${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

target_link_libraries(practica-final PRIVATE fmt::fmt
    SFML::System SFML::Graphics SFML::Window)
