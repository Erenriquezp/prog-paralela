#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <complex>

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef LINUX
    #include <unistd.h>
#endif

// dimensiones de la ventana
#define WIDTH 1600
#define HEIGHT 900

// parametros del fractal
int max_iterations = 10;

double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.0;
double y_max = 1.0;

std::complex<double> c(-0.7, 0.27015); // c es el parametro del fractal, se puede cambiar para obtener diferentes formas

// textura
uint32_t* pixel_buffer = nullptr; // entero de 32 bits para almacenar el color de cada pixel (RGBA)
uint16_t* iter_buffer = nullptr;

int main() {

    pixel_buffer = new uint32_t[WIDTH * HEIGHT];
    
    sf::RenderWindow window( sf::VideoMode({WIDTH, HEIGHT}), "Fractal Julia set - FML");
#ifdef _WIN32
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif
    while (window.isOpen()) {

        while (const std::optional event = window.pollEvent()) {
            if (event -> is<sf::Event::Closed>()) {
                window.close();
            }
        }

        //dibujar fractal

        window.clear();
        window.display();
    }

    return 0;
}