cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)

add_executable(FractalJulia main.cpp fractal_serial.cpp)

target_link_libraries(FractalJulia PRIVATE fmt::fmt 
    SFML::System SFML::Graphics SFML::Window )
