#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <complex>

#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <mpi.h>

#include "fractal.h"
#include "palette.h"
#include "draw_text.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace arial_ttf
{
    extern size_t data_len;
    extern unsigned char data[];
}

#define WIDTH 1600
#define HEIGHT 900

uint32_t* pixel_buffer = nullptr;
uint32_t* texture_buffer = nullptr;

int running = 1;

int nprocs;
int rank;

int row_start;
int row_end;
int delta;
int padding;

int max_iteraciones = 10;

double x_min = -2;
double x_max = 1;
double y_min = -1.5;
double y_max = 1.5;

std::complex<double> c(-0.7, 0.27015);

enum runtime_type {
    SERIAL_1 = 0,
    SERIAL_2,
    SIMD,
    OPENMP_REGIONES,
    OPENMP_FOR,
    OPENMP_FOR_SIMD
};

static int rows_for_rank(int r)
{
    int start = r * delta;
    if (start >= HEIGHT) return 0;
    int end = start + delta;
    if (end > HEIGHT) end = HEIGHT;
    return end - start;
}

void dibujar_texto(int rank_value)
{
    auto texto = " ";
    (void)texto;
}

void setup_iu()
{
    texture_buffer = new uint32_t[WIDTH * HEIGHT];
    std::memset(texture_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));

    sf::RenderWindow window(sf::VideoMode({(unsigned)WIDTH, (unsigned)HEIGHT}), "Fractal MPI");

#ifdef _WIN32
    HWND hWnd = window.getNativeHandle();
    ShowWindow(hWnd, SW_MAXIMIZE);
#endif

    sf::Texture texture;
    if (!texture.resize({WIDTH, HEIGHT})) {
        fmt::println("Error");
        running = 0;
        return;
    }

    sf::Sprite sprite(texture);

    const sf::Font font(arial_ttf::data, arial_ttf::data_len);
    sf::Text text(font, "Fractal", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    std::string options = "Options: Up/Down: Change iterations";
    sf::Text textOptions(font, options, 20);
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getView().getSize().y - 40});

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
                switch (evt->scancode) {
                    case sf::Keyboard::Scan::Up:
                        max_iteraciones += 10;
                        break;
                    case sf::Keyboard::Scan::Down:
                        max_iteraciones -= 10;
                        if (max_iteraciones < 10) max_iteraciones = 10;
                        break;
                    default:
                        break;
                }
                std::memset(pixel_buffer, 0, WIDTH * delta * sizeof(uint32_t));
            }
        }

        std::array<int, 2> state = {max_iteraciones, running};
        std::array<double, 4> plane = {x_min, y_min, x_max, y_max};

        MPI_Bcast(state.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(plane.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        max_iteraciones = state[0];
        running = state[1];
        x_min = plane[0];
        y_min = plane[1];
        x_max = plane[2];
        y_max = plane[3];

        if (running == 0) {
            break;
        }

        burning_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer);
        dibujar_texto(rank);

        int my_rows = rows_for_rank(rank);
        if (my_rows > 0) {
            std::memcpy(texture_buffer + row_start * WIDTH, pixel_buffer, my_rows * WIDTH * sizeof(uint32_t));
        }

        for (int src = 1; src < nprocs; ++src) {
            int src_rows = rows_for_rank(src);
            int src_start = src * delta;

            MPI_Recv(
                pixel_buffer,
                WIDTH * src_rows,
                MPI_UINT32_T,
                src,
                0,
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE
            );

            if (src_rows > 0) {
                std::memcpy(texture_buffer + src_start * WIDTH, pixel_buffer, src_rows * WIDTH * sizeof(uint32_t));
            }
        }

        texture.update((const std::uint8_t*)texture_buffer);

        frames++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        auto msg = fmt::format("Fractal: Iterations: {}, FPS: {}, Mode: MPI", max_iteraciones, fps);
        text.setString(msg);

        window.clear();
        window.draw(sprite);
        window.draw(text);
        window.draw(textOptions);
        window.display();
    }

    running = 0;

    std::array<int, 2> state = {max_iteraciones, running};
    std::array<double, 4> plane = {x_min, y_min, x_max, y_max};

    MPI_Bcast(state.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(plane.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

int main(int argc, char** argv)
{
    init_freetype();

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    delta = std::ceil(HEIGHT * 1.0 / nprocs);
    row_start = rank * delta;
    row_end = row_start + delta;
    padding = delta * nprocs - HEIGHT;

    if (row_end > HEIGHT) {
        row_end = HEIGHT;
    }

    pixel_buffer = new uint32_t[WIDTH * delta];
    std::memset(pixel_buffer, 0, WIDTH * delta * sizeof(uint32_t));

    fmt::println("RANK: {}, rows: {}, to: {}", rank, row_start, row_end);

    if (rank == 0) {
        setup_iu();
    } else {
        while (true) {
            std::array<int, 2> state;
            std::array<double, 4> plane;

            MPI_Bcast(state.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(plane.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            max_iteraciones = state[0];
            running = state[1];
            x_min = plane[0];
            y_min = plane[1];
            x_max = plane[2];
            y_max = plane[3];

            if (running == 0) {
                break;
            }

            burning_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer);
            dibujar_texto(rank);

            int my_rows = row_end - row_start;
            if (my_rows < 0) {
                my_rows = 0;
            }

            MPI_Send(
                pixel_buffer,
                WIDTH * my_rows,
                MPI_UINT32_T,
                0,
                0,
                MPI_COMM_WORLD
            );
        }
    }

    delete[] pixel_buffer;
    if (rank == 0) delete[] texture_buffer;

    MPI_Finalize();
    return 0;
}