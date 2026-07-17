#include <cmath>
#include <cstdint>
#include <cstring>

#include <fmt/core.h>
#include <cuda_runtime.h>
#include <SFML/Graphics.hpp>
#include "palette.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

#define WIDTH 1600
#define HEIGHT 900
#define NBINS 16

int max_iteraciones = 100;

double x_min = -1.8;
double x_max = -1.7;
double y_min = -0.1;
double y_max = 0.05;

uint32_t* host_pixels = nullptr;
uint32_t* device_pixels = nullptr;

int* d_hist = nullptr;
int h_hist[NBINS];

extern "C" void copiar_paleta(unsigned int* h_palette);
extern "C" void burning_ship_gpu(int max_iter,
                                  double x_min, double y_min, double x_max, double y_max,
                                  uint32_t width, uint32_t height,
                                  uint32_t* device_pixels, int* d_hist);

#define CHECK(expr) {                               \
        auto err = (expr);                          \
        if (err != cudaSuccess) {                   \
            fmt::println("CUDA Error {}: {} at line {}", (int)err, cudaGetErrorString(err), __LINE__); \
            exit(EXIT_FAILURE);                     \
        }                                           \
    }

int main()
{
    int deviceId = 0;
    cudaSetDevice(deviceId);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, deviceId);
    fmt::println("Device: {}", prop.name);
    fmt::println("Memory: {:.0f} MB", prop.totalGlobalMem / 1024.0 / 1024.0);
    fmt::println("SM count: {}", prop.multiProcessorCount);
    fmt::println("Max threads/block: {}", prop.maxThreadsPerBlock);

    size_t buffer_size = WIDTH * HEIGHT * sizeof(uint32_t);
    host_pixels = (uint32_t*)malloc(buffer_size);
    std::memset(host_pixels, 0, buffer_size);

    CHECK(cudaMalloc(&device_pixels, buffer_size));
    CHECK(cudaMalloc(&d_hist, NBINS * sizeof(int)));

    copiar_paleta(color_ramp.data());

    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Burning Ship CUDA");

#ifdef _WIN32
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif

    sf::Texture texture(sf::Vector2u(WIDTH, HEIGHT));
    sf::Sprite sprite(texture);

    sf::Font font("arial.ttf");
    sf::Text text(font, "Burning Ship", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    std::string options = "Up/Down: Iterations, Esc: Close";
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
                        if (max_iteraciones < 10) max_iteraciones = 10;
                        break;
                    case sf::Keyboard::Scan::Escape:
                        window.close();
                        break;
                    default:
                        break;
                }
                std::memset(host_pixels, 0, buffer_size);
            }
        }

        burning_ship_gpu(max_iteraciones,
                         x_min, y_min, x_max, y_max,
                         WIDTH, HEIGHT, device_pixels, d_hist);
        CHECK(cudaGetLastError());

        CHECK(cudaMemcpy(host_pixels, device_pixels, buffer_size, cudaMemcpyDeviceToHost));
        CHECK(cudaMemcpy(h_hist, d_hist, NBINS * sizeof(int), cudaMemcpyDeviceToHost));

        texture.update((const uint8_t*)host_pixels);

        frames++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        std::string hist_str = "Hist:[";
        for (int i = 0; i < NBINS; ++i) {
            if (i > 0) hist_str += ",";
            hist_str += fmt::format("{}", h_hist[i]);
        }
        hist_str += "]";

        auto msg = fmt::format("Burning Ship CUDA | Iter: {} | FPS: {} | GPU: {}\n{}",
            max_iteraciones, fps, prop.name, hist_str);
        text.setString(msg);

        window.clear();
        window.draw(sprite);
        window.draw(text);
        window.draw(textOptions);
        window.display();
    }

    free(host_pixels);
    cudaFree(device_pixels);
    cudaFree(d_hist);

    return 0;
}
