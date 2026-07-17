#include <iostream>
#include <cmath>
#include <fmt/core.h>
#include <cuda_runtime.h>
#include <complex>
#include "palette.h" // Contiene 'color_ramp' (std::vector) y 'PALETTE_SIZE'

#include <SFML/Graphics.hpp>

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef _WIN32
    // Fuerza el uso de la GPU dedicada de NVIDIA
    extern "C" {
        _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    }
#endif

#define WIDTH 1920
#define HEIGHT 1080

// Parámetros globales
int max_iteraciones = 10;

double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.0;
double y_max = 1.0;

std::complex<double> c(-0.7, 0.27015);

uint32_t* host_pixel_buffer = nullptr;
uint32_t* device_pixel_buffer = nullptr;

// DECLARACIONES EXTERNAS DE LAS FUNCIONES DE LA GPU (mismo orden que en kernel.cu)
extern "C" void copiar_paleta(unsigned int* h_palette);
// --- CORRECCIÓN DE LA DECLARACIÓN (Línea 32-35) ---
extern "C" void julia_gpu(double centro_real, double centro_img, int num_iteraciones, 
                          double x_min, double y_min, double x_max, double y_max, 
                          uint32_t width, uint32_t height, uint32_t* pixel_buffer);

// Verificar errores de CUDA
#define CHECK(expr) {                               \
        auto internal_error = (expr);               \
        if (internal_error!=cudaSuccess) {          \
            fmt::println("{}: {} in {} at line {}", (int)internal_error, cudaGetErrorString(internal_error), __FILE__, __LINE__);    \
            exit(EXIT_FAILURE);                     \
        }                                           \
    }

int main() {
    int deviceId = 0;
    cudaSetDevice(deviceId);

    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, deviceId);

    fmt::println("Device: {}", deviceProp.name);
    fmt::println("Total memory: {:.2f} MB", deviceProp.totalGlobalMem / 1024.0 / 1024.0);

    // Inicializar buffers de píxeles
    size_t buffer_size = WIDTH * HEIGHT * sizeof(uint32_t);
    host_pixel_buffer = (uint32_t*) malloc(buffer_size);
    std::memset(host_pixel_buffer, 0, buffer_size);

    CHECK(cudaMalloc(&device_pixel_buffer, buffer_size));

    // CORRECCIÓN: Copiar la paleta del std::vector de la CPU a la memoria constante de la GPU
    // (Asumo que tu 'palette.h' ya rellena el vector 'color_ramp' al incluirlo o tienes una función para ello)
    copiar_paleta(color_ramp.data());

    // Inicializar UI de SFML 3
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Fractal Julia en GPU");
    
#ifdef _WIN32    
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif

    // CORRECCIÓN: Inicialización de textura compatible con SFML 3 sin ambigüedades
    sf::Texture texture(sf::Vector2u(WIDTH, HEIGHT));    
    sf::Sprite sprite(texture);
    
    sf::Font font("arial.ttf");
    sf::Text text(font, "Julia Set", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    // Texto de opciones actualizado y simplificado para el modo GPU
    std::string options = "Controles: [Flecha Arriba] +Iteraciones | [Flecha Abajo] -Iteraciones";
    sf::Text textOptions(font, options, 20);
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getView().getSize().y - 40});

    int frames = 0;
    sf::Clock clock;
    int fps = 0;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (event->is<sf::Event::KeyReleased>()) {
                auto evt = event->getIf<sf::Event::KeyReleased>();

                switch (evt->scancode) {
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
                // Limpiar buffer local al cambiar parámetros para evitar artefactos visuales transitorios
                std::memset(host_pixel_buffer, 0, buffer_size);
            }
        }
        
        std::string mode = "GPU CUDA";

        // 1. Ejecutar el cálculo del fractal en la GPU
        julia_gpu(c.real(), c.imag(), max_iteraciones, 
                  x_min, y_min, x_max, y_max, 
                  WIDTH, HEIGHT, device_pixel_buffer);
        CHECK(cudaGetLastError());

        // 2. Copiar los datos calculados de la GPU de vuelta al Host (CPU)
        CHECK(cudaMemcpy(host_pixel_buffer, device_pixel_buffer, buffer_size, cudaMemcpyDeviceToHost));
        
        // 3. Actualizar la textura gráfica de SFML
        texture.update((const uint8_t *) host_pixel_buffer);

        // Contar FPS de forma precisa
        frames++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // Actualizar la HUD informativa
        auto msg = fmt::format("Julia Set | Iterations: {} | FPS: {} | Mode: {}", max_iteraciones, fps, mode);
        text.setString(msg);

        // Renderizado en pantalla
        window.clear();
        window.draw(sprite);
        window.draw(text);
        window.draw(textOptions);
        window.display();
    }
    
    // Liberación limpia de recursos al cerrar el programa
    free(host_pixel_buffer);
    cudaFree(device_pixel_buffer);
    
    return 0;
}