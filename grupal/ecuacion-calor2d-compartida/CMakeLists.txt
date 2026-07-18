cmake_minimum_required(VERSION 3.16)

project(EcuacionCalor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -fopenmp") 

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(
    EcuacionCalor 
    main.cpp 
    calor_serial.cpp 
    palette.cpp 
    calor_simd.cpp
    calor_openmp.cpp
)

target_link_libraries(EcuacionCalor PRIVATE 
    fmt::fmt 
    SFML::System SFML::Graphics SFML::Window 
)

