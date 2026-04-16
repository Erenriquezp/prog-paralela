#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <optional> // Necesario para el nuevo sistema de eventos

int main() {
    // 1. Verificar hilos (importante para tu materia de Paralela) [cite: 5]
    int max_threads = omp_get_max_threads();
    fmt::print("Sistema listo. Hilos disponibles: {}\n", max_threads);

    // 2. Crear ventana: SFML 3 usa un Vector2u para el tamaño en el constructor
    sf::RenderWindow window(sf::VideoMode({400, 200}), "UCE - Prog. Paralela");
    
    while (window.isOpen()) {
        // 3. Manejo de eventos: SFML 3 ahora devuelve un std::optional
        while (const std::optional event = window.pollEvent()) {
            // Verificamos si el evento es de tipo "Cerrar"
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color(30, 30, 30));
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