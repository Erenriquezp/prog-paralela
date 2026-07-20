cmake_minimum_required(VERSION 3.16)

project(PruebaCuda)

enable_language(CXX CUDA)

find_package(fmt CONFIG REQUIRED)
#find_package(Stb REQUIRED)
#find_package(Freetype REQUIRED)

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)

#--
add_executable(prueba_cuda
    main.cpp
    kernel.cu

    )

target_link_libraries(prueba_cuda  PRIVATE 
    fmt::fmt
    SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)

