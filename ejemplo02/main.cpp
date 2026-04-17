#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <optional> // Necesario para el nuevo sistema de eventos

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "UCE - Prog. Paralela");
    
    const sf::Font font("arial.ttf");
    sf::Text text(font, "Hello SFML", 50);

    while (window.isOpen()) {
        // 3. Manejo de eventos: SFML 3 ahora devuelve un std::optional
        while (const std::optional event = window.pollEvent()) {
            // Verificamos si el evento es de tipo "Cerrar"
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color(30, 30, 30));
        window.draw(text);
        window.display();
    }
    return 0;
}

// #include <iostream>
// #include <fmt/core.h>

// int  main(){
//     std::printf("Hola, mundo\n");
//     fmt::println("Hola, mundo\n");

//     return 0;
// };