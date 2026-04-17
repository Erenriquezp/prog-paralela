cmake_minimum_required(VERSION 3.16)
project(FractalJulia)

# vcpkg instaló SFML 3.0.2, que requiere nombres con mayúsculas
find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

add_executable(fractal-julia 
    main.cpp
    fractal_serial.cpp
)

# Enlazar usando los targets de SFML 3
target_link_libraries(fractal-julia PRIVATE 
    fmt::fmt 
    SFML::Graphics 
    SFML::Window 
    SFML::System
)