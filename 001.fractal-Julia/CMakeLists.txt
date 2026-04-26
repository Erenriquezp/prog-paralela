cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(FractalJulia 
    main.cpp 
    fractal_serial.cpp 
    palette.cpp
    fractal_simd.cpp
)

target_link_libraries(FractalJulia PRIVATE fmt::fmt 
    SFML::System SFML::Graphics SFML::Window )
