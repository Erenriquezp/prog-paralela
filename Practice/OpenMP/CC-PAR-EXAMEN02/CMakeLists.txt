cmake_minimum_required(VERSION 3.16)

project(Filtro)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -fopenmp")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(Filtro 
    main.cpp 
    filtro.cpp
)

target_link_libraries(Filtro PRIVATE fmt::fmt 
    SFML::System SFML::Graphics SFML::Window )
