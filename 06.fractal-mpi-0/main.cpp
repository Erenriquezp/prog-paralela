#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <mpi.h>
#include <complex>
#include <optional>

#include "fractal_mpi.h"

#ifdef _WIN32
    #include <windows.h>
#endif

// -- parametros Julia
double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.0;
double y_max = 1.0;

int max_iteraciones = 10;

std::complex<double> c(-0.7, 0.27015);

#define WIDTH 1600
#define HEIGHT 900

uint32_t* pixel_buffer = nullptr;
uint32_t* texture_buffer = nullptr; // solo rank 0

int running = 1;

void setup_iu(int nprocs, int delta, int row_start, int row_end) {
    // El gather junta nprocs franjas de 'delta' filas; reservamos ese tamaño (>= WIDTH*HEIGHT)
    texture_buffer = new uint32_t[WIDTH * delta * nprocs];
    std::memset(texture_buffer, 0, WIDTH * delta * nprocs * sizeof(uint32_t)); // inicializamos

    // crear la UI
    sf::RenderWindow window(sf::VideoMode({(unsigned)WIDTH, (unsigned)HEIGHT}), "Fractal MPI");

    #ifdef _WIN32
        HWND hWnd = window.getNativeHandle();
        ShowWindow(hWnd, SW_MAXIMIZE);
    #endif

    sf::Texture texture;
    texture.resize({WIDTH, HEIGHT});
    sf::Sprite sprite(texture);
    sf::Font font;
    font.openFromFile("arial.ttf");
    sf::Text text(font, "Julia Set", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    std::string options = "Options: Up/Down: Change iterations";
    sf::Text textOptions(font, options, 20);
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getView().getSize().y -40});

    int frames = 0;
    sf::Clock clock;
    int fps = 0;

    while (window.isOpen())
    {
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) {
                running = 0;
                window.close();
            }

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
                }
                // TODO: reactivar al dibujar el fractal. Desborda pixel_buffer (rank 0 lo reserva como WIDTH*delta, no WIDTH*HEIGHT).
                // std::memset(pixel_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));
            }
        }

        // si la ventana se cerró, salimos sin lanzar otra ronda colectiva
        if (!window.isOpen()) break;

        // 1) enviar los parámetros de control a los workers
        std::vector<int> dummy = {max_iteraciones, running};
        MPI_Bcast(dummy.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);

        // 2) el master calcula su propia franja
        julia_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer);

        // 3) recolectar las franjas de todos los ranks en texture_buffer
        MPI_Gather(pixel_buffer, WIDTH * delta, MPI_UINT32_T,
                   texture_buffer, WIDTH * delta, MPI_UINT32_T,
                   0, MPI_COMM_WORLD);

        // 4) volcar el resultado a la textura
        texture.update((const uint8_t *) texture_buffer);

        // contar FPS
        frames++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        window.clear();
        {
            window.draw(sprite);
            window.draw(text);
            window.draw(textOptions);
        }
        window.display();
    }

    // avisar a los workers que deben terminar (tras esto no habrá gather)
    running = 0;
    std::vector<int> dummy = {max_iteraciones, running};
    MPI_Bcast(dummy.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nprocs;
    int rank;
    
    // ranks 
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int delta = std::ceil(HEIGHT*1.0 / nprocs); // 1600/4 = 400
    /**
     * r0: start = 0*400 = 0, end = 0 +400 = 400
     * r1: start = 1*400 = 400, end = 400 +400 = 800
     * r2: start = 2*400 = 800, end = 800 +400 = 1200
     * r3: start = 3*400 = 1200, end = 1200 +400 = 1600
     */

    int row_start = rank * delta;
    int row_end = row_start + delta;
    int padding = delta * nprocs - HEIGHT; // 400 * 4 - 1600 = 0

    if(row_end > HEIGHT) {
        row_end = HEIGHT;
    }

    pixel_buffer = new uint32_t[WIDTH * delta];
    std::memset(pixel_buffer, 0, WIDTH * delta * sizeof(uint32_t)); // inicializamos

    fmt::print("RANK: {}, rows: {}, to: {}\n", rank, row_start, row_end);

    if (rank == 0) {
        // master
        setup_iu(nprocs, delta, row_start, row_end);
    } else {
        // workers
        while (true) {
            // recibir los parámetros de control del master
            std::vector<int> dummy(2);
            MPI_Bcast(dummy.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
            max_iteraciones = dummy[0];
            running = dummy[1];

            if (running == 0) {
                fmt::print("RANK {}: Received shutdown signal. Exiting.\n", rank);
                break;
            }

            julia_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer);

              if (rank == 1)
            {
                fmt::print("rank: {}: done computing rows {} to {}\n", rank, row_start, row_end);
            }

            MPI_Gather(pixel_buffer, WIDTH * delta, MPI_UINT32_T,
                       nullptr, WIDTH * delta, MPI_UINT32_T,
                       0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    // C:\oneAPI>setvars
    //C:\tools\prog-paralela\05.ejemplo-mpi\build\Debug>mpiexec -n 4 fractal-mpi.exe
    return 0;
}