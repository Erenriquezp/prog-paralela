cmake_minimum_required(VERSION 3.16)

project(FiltroPractice)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -fopenmp")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(FiltroPractice
   main.cpp 
   test.cpp
   producto.cpp
   practice.cpp
   matriz.cpp
)

target_link_libraries(FiltroPractice PRIVATE fmt::fmt 
    SFML::System SFML::Graphics SFML::Window )