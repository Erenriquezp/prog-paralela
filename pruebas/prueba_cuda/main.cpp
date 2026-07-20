#include <fmt/core.h>
#include <cuda_runtime.h>
#include <complex>
#include <SFML/Graphics.hpp>
#include <filesystem>

#include "arial.ttf.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define WIDTH 6016
#define HEIGHT 4016

uint32_t *host_pixel_buffer = nullptr;
uint32_t *device_pixel_buffer = nullptr;

uint32_t *device_processed_buffer = nullptr;

bool aplicar_efecto = false;
sf::Image im;
uint32_t im_width = 0;
uint32_t im_height = 0;

extern "C" void aplicar_realzamiento(
    uint32_t width, uint32_t height,
    uint32_t *d_input_buffer,
    uint32_t *d_output_buffer);

#define CHECK(expr)                                                                                                               \
    {                                                                                                                             \
        auto internal_error = (expr);                                                                                             \
        if (internal_error != cudaSuccess)                                                                                        \
        {                                                                                                                         \
            fmt::println("{}: {} in {} at line {}", (int)internal_error, cudaGetErrorString(internal_error), __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                                                                   \
        }                                                                                                                         \
    }

int main()
{
    int diviceId = 0;
    CHECK(cudaSetDevice(diviceId));

    cudaDeviceProp props;
    CHECK(cudaGetDeviceProperties(&props, diviceId));

    fmt::println("Device: {}", props.name);
    fmt::println("Total Memory: {} MB", props.totalGlobalMem / 1024.0 / 1024.0);
    fmt::println("Multiprocessor Count: {}", props.multiProcessorCount);
    fmt::println("Max Threads per Multiprocessor: {}", props.maxThreadsPerMultiProcessor);
    fmt::println("Max Threads per Block: {}", props.maxThreadsPerBlock);
    fmt::println("Max Blocks Per Grid: {}", props.maxGridSize[0]);
    fmt::println("mMax threads per multiprocessor: {}", props.maxThreadsPerMultiProcessor);

    size_t buffer_size = WIDTH * HEIGHT * sizeof(uint32_t);
    host_pixel_buffer = (uint32_t *)malloc(buffer_size);

    std::memset(host_pixel_buffer, 0, buffer_size);

    CHECK(cudaMalloc((void **)&device_pixel_buffer, buffer_size));
    CHECK(cudaMalloc((void **)&device_processed_buffer, buffer_size));

    // cargar imagen
    std::string filename = "image01.jpg";
    if (!im.loadFromFile(filename))
    {
        fmt::println("no se encontro la imagen", filename);
        return 1;
    }

    im_width = im.getSize().x;
    im_height = im.getSize().y;
    fmt::println("imagen cargada: {}*{}", im_width, im_height);

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Realzar Bordes");

#ifdef _WIN32
    
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif

    sf::Texture texture(sf::Vector2u(WIDTH, HEIGHT), false);
    texture.update((const uint8_t *)host_pixel_buffer);

    sf::Sprite sprite(texture);

    // texto
    const sf::Font font(arial_ttf, sizeof(arial_ttf));
    sf::Text text(font, "en carga ...", 20);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    std::string options = " B: realzar bordes - R: original ";

    sf::Text textOptions(font, options, 100);
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getView().getSize().y - 120});

    while (window.isOpen())
    {
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            else if (event->is<sf::Event::KeyReleased>())
            {
                auto evt = event->getIf<sf::Event::KeyReleased>();

                switch (evt->scancode)
                {
                case sf::Keyboard::Scan::B:
                    aplicar_efecto = true;
                    break;
                case sf::Keyboard::Scan::R:
                    aplicar_efecto = false;
                    break;
                }
                std::memset(host_pixel_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));
            }
        }

        std::string mode = "GPU";

        // copiar píxeles de la imagen cargada a host
        const uint8_t* ptr = im.getPixelsPtr();
        size_t img_buffer_size = im_width * im_height * 4; // RGBA
        std::memcpy(host_pixel_buffer, ptr, std::min(img_buffer_size, buffer_size));

        // copiar imagen a device
        CHECK(cudaMemcpy(device_pixel_buffer, host_pixel_buffer, buffer_size, cudaMemcpyHostToDevice));

        // aplicar realce de bordes
        if (aplicar_efecto)
        {
            aplicar_realzamiento(im_width, im_height, device_pixel_buffer, device_processed_buffer);
            CHECK(cudaGetLastError());
        }

        CHECK(cudaDeviceSynchronize());

        // copar a host
        uint32_t *source_buffer = aplicar_efecto ? device_processed_buffer : device_pixel_buffer;
        CHECK(cudaMemcpy(host_pixel_buffer, source_buffer, buffer_size, cudaMemcpyDeviceToHost));

        texture.update((const uint8_t *)host_pixel_buffer);

        window.clear();
        {
            window.draw(sprite);
            window.draw(text);
            window.draw(textOptions);
        }
        window.display();
    }

    free(host_pixel_buffer);

    CHECK(cudaFree(device_pixel_buffer));
    CHECK(cudaFree(device_processed_buffer));

    return 0;
}
