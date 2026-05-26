#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "filtro.h"

enum class ModoVisualizacion {
    ORIGINAL,
    FILTRO_SIMD,
    FILTRO_OPENMP
};

int main() {
    int width, height, channels;
    uint8_t* rgba_pixels = stbi_load("img.jpg", &width, &height, &channels, STBI_rgb_alpha);
    
    if (!rgba_pixels) {
        fmt::print("Error crítico: No se pudo cargar la imagen 'img.jpg'\n");
        return -1;
    }

    int total_píxeles = width * height;
    uint8_t* gray_pixels = new uint8_t[total_píxeles];
    uint8_t* sfml_render_buffer = new uint8_t[total_píxeles * 4];

    ModoVisualizacion modo_actual = ModoVisualizacion::ORIGINAL;
    double tiempo_proceso_ms = 0.0;

    sf::RenderWindow window(sf::VideoMode({(unsigned int)(width), (unsigned int)(height)}), "UCE - Filtro Escala de Grises");
    
    sf::Texture texture({(unsigned int)(width), (unsigned int)(height)});
    sf::Sprite sprite(texture);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (event->is<sf::Event::KeyReleased>()) {
                auto evt = event->getIf<sf::Event::KeyReleased>();
                                
                switch (evt->scancode) {
                    case sf::Keyboard::Scan::Num1:
                        modo_actual = ModoVisualizacion::ORIGINAL;
                        tiempo_proceso_ms = 0.0;
                        break;
                    case sf::Keyboard::Scan::Num2:
                        filtro_escala_grises_simd(rgba_pixels, gray_pixels, width, height);
                        modo_actual = ModoVisualizacion::FILTRO_SIMD;
                        break;
                    case sf::Keyboard::Scan::Num3:
                        filtro_escala_grises_regiones(rgba_pixels, gray_pixels, width, height);
                        modo_actual = ModoVisualizacion::FILTRO_OPENMP;
                        break;
                    case sf::Keyboard::Scan::S:
                        if (modo_actual == ModoVisualizacion::FILTRO_SIMD) {
                            stbi_write_png("salida_simd.png", width, height, STBI_grey, gray_pixels, width);
                            fmt::print("Imagen guardada exitosamente como 'salida_simd.png'\n");
                        } else if (modo_actual == ModoVisualizacion::FILTRO_OPENMP) {
                            stbi_write_png("salida_openmp.png", width, height, STBI_grey, gray_pixels, width);
                            fmt::print("Imagen guardada exitosamente como 'salida_openmp.png'\n");
                        }
                        break;
                }
            }
        }

        if (modo_actual == ModoVisualizacion::ORIGINAL) {
            texture.update(rgba_pixels);
        } else {
            for (int i = 0; i < total_píxeles; i++) {
                int idx = i * 4;
                uint8_t g = gray_pixels[i];
                sfml_render_buffer[idx]     = g; // R
                sfml_render_buffer[idx + 1] = g; // G
                sfml_render_buffer[idx + 2] = g; // B
                sfml_render_buffer[idx + 3] = 255; // A (Opaco)
            }
            texture.update(sfml_render_buffer);
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    stbi_image_free(rgba_pixels);
    delete[] gray_pixels;
    delete[] sfml_render_buffer;

    return 0;
}