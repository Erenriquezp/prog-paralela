cmake_minimum_required(VERSION 3.16)
project(Ejemplo01)

# vcpkg instaló SFML 3.0.2, que requiere nombres con mayúsculas
find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

add_executable(Ejemplo01 main.cpp)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(Ejemplo01 PRIVATE OpenMP::OpenMP_CXX)
endif()

# Enlazar usando los targets de SFML 3
target_link_libraries(Ejemplo01 PRIVATE 
    fmt::fmt 
    SFML::Graphics 
    SFML::Window 
    SFML::System
)