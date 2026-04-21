#include <fmt/core.h>
#include <complex>
#include <SFML/Graphics.hpp>
#include "fractal_serial.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#define WIDTH 1600
#define HEIGHT 900

//parametros
int max_iteraciones = 10;

double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.0;
double y_max = 1;

std::complex<double> c(-0.7, 0.27015);

// textura
uint32_t* pixel_buffer = nullptr;
//uint16_t* iter_buffer = nullptr;

int main(){

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
                
                default:
                    break;
                }
            }
        }

        // dibujar
        //julia_serial_1(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);
        julia_serial_2(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, pixel_buffer);


        texture.update((const uint8_t *) pixel_buffer);

        // contar FPS
        frames++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // actualizar el titulo de la ventana con el FPS
        auto msg = fmt::format("Julia Set: Iteraciones {}, FPS: {}", max_iteraciones, fps);
        text.setString(msg);

        window.clear();
        {
            window.draw(sprite);
            window.draw(text);
        }
        window.display();
    }
    
    delete[] pixel_buffer;
    
    return 0;
}