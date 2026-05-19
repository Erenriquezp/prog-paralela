#include <fmt/core.h>
#include <complex>
#include <SFML/Graphics.hpp>
#include <omp.h>
#include "fractal_serial.h"
#include "fractal_simd.h"

#include "fractal_openmp.h"

// RAMP Generator
// https://www.rampgenerator.com/ - para generar colores

#ifdef _WIN32
    #include <windows.h>
#endif

// Parámetros de la ventana, en este caso igual a la resolucion de la imagen
#define WIDTH 1920
#define HEIGHT 1080

//parametros
int max_iteraciones = 10;

double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.0;
double y_max = 1.0;

std::complex<double> c(-0.7, 0.27015);

// textura
uint32_t* pixel_buffer = nullptr;
//uint16_t* iter_buffer = nullptr;

enum runtime_type {
    SERIAL_1 = 0,
    SERIAL_2,
    SIMD,
    OPENMP_REGIONES,
    OPENMP_FOR,
    OPENMP_FOR_SIMD
};

int main(){

    int thread_count;
    #pragma omp parallel
    {
        #pragma omp master
        thread_count = omp_get_num_threads();
    }

    runtime_type r_type = runtime_type::SERIAL_1;

    pixel_buffer = new uint32_t[WIDTH * HEIGHT];
    // Create the main window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Julia");

#ifdef _WIN32    
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif

    sf::Texture texture({WIDTH, HEIGHT});
    sf::Sprite sprite(texture);
    sf::Font font("arial.ttf");
    sf::Text text(font, "Julia Set", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    std::string options = "Options: [1] Serial 1 [2] Serial 2 [3] SIMD [4] OpenMP Regiones [5] OpenMP For | Up/Down: Change iterations";
    sf::Text textOptions(font, options, 20);
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getView().getSize().y -40});

    int frames = 0;
    sf::Clock clock;
    int fps = 0;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (event->is<sf::Event::KeyReleased>()) {
                auto evt = event->getIf<sf::Event::KeyReleased>();

                switch (evt -> scancode) {
                    case sf::Keyboard::Scan::Up:
                        max_iteraciones += 10;
                        break;
                    case sf::Keyboard::Scan::Down:
                        max_iteraciones -= 10;
                        if (max_iteraciones < 10) {
                            max_iteraciones = 10;
                        }
                        break;
                    case sf::Keyboard::Scan::Num1:
                        r_type = runtime_type::SERIAL_1;
                        break;
                    case sf::Keyboard::Scan::Num2:
                        r_type = runtime_type::SERIAL_2;
                        break;
                    case sf::Keyboard::Scan::Num3:
                        r_type = runtime_type::SIMD;
                        break;
                    case sf::Keyboard::Scan::Num4:
                        r_type = runtime_type::OPENMP_REGIONES;
                        break;
                    case sf::Keyboard::Scan::Num5:
                        r_type = runtime_type::OPENMP_FOR;
                        break;
                    case sf::Keyboard::Scan::Num6:
                        r_type = runtime_type::OPENMP_FOR_SIMD;
                        break;
                }
                std::memset(pixel_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));
            }
        }

        // dibujar
        //julia_serial_1(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
        //julia_serial_2(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);

        std::string mode = " ";
        if (r_type == runtime_type::SERIAL_1) {
            julia_serial_1(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
            mode = "Serial 1";
        } else if (r_type == runtime_type::SERIAL_2){
            julia_serial_2(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
            mode = "Serial 2";
        } else if (r_type == runtime_type::SIMD) {
            julia_simd(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
            mode = "SIMD";
        } else if (r_type == runtime_type::OPENMP_REGIONES) {
            julia_openmp_regiones(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
            mode = fmt::format("OpenMP Regiones ({} threads)", thread_count);
        } else if (r_type == runtime_type::OPENMP_FOR) {
            julia_openmp_for(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
            mode = fmt::format("OpenMP For ({} threads)", thread_count);
        } else if (r_type == runtime_type::OPENMP_FOR_SIMD) {
            julia_openmp_for_simd(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
            mode = fmt::format("OpenMP For SIMD ({} threads)", thread_count);
        }

        texture.update((const uint8_t *) pixel_buffer);

        // contar FPS
        frames++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // actualizar el titulo de la ventana con el FPS
        //auto msg = fmt::format("Julia Set: Iteraciones {}, FPS: {}", max_iteraciones, fps);
        auto msg = fmt::format("Julia Set: Iterations: {} | FPS: {}, | Mode: {}", max_iteraciones, fps, mode);
        text.setString(msg);

        window.clear();
        {
            window.draw(sprite);
            window.draw(text);
            window.draw(textOptions);
        }
        window.display();
    }
    
    delete[] pixel_buffer;
    
    return 0;
}