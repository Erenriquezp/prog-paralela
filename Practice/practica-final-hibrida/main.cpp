#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <mpi.h>
#include <cuda_runtime.h>

#include "fractal.h"
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

uint32_t* pixel_buffer = nullptr;
uint32_t* texture_buffer = nullptr;

uint32_t* device_pixels = nullptr;
long long* d_iters = nullptr;

int running = 1;
int current_mode = MODE_SERIAL; // Default mode

int nprocs;
int rank;

int row_start;
int row_end;
int delta;
int padding;

int max_iteraciones = 50;

double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.0;
double y_max = 1.0;

double local_time_ms = 0.0;
double max_time_ms = 0.0;
long long local_iters = 0;
long long total_iters = 0;

static int rows_for_rank(int r)
{
    int start = r * delta;
    if (start >= HEIGHT) return 0;
    int end = start + delta;
    if (end > HEIGHT) end = HEIGHT;
    return end - start;
}

void setup_iu()
{
    texture_buffer = new uint32_t[WIDTH * HEIGHT];
    std::memset(texture_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));

    sf::RenderWindow window(sf::VideoMode({(unsigned)WIDTH, (unsigned)HEIGHT}), "Fractal de Newton - Hibrido");

#ifdef _WIN32
    HWND hWnd = window.getNativeHandle();
    ShowWindow(hWnd, SW_MAXIMIZE);
#endif

    sf::Texture texture(sf::Vector2u(WIDTH, HEIGHT));
    sf::Sprite sprite(texture);

    sf::Font font("arial.ttf");
    sf::Text text(font, "Fractal de Newton", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    std::string options = "Controles: Up/Down: Iteraciones | S: Serial | O: OpenMP | C: CUDA | Esc: Salir";
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
                    case sf::Keyboard::Scan::S:
                        current_mode = MODE_SERIAL;
                        break;
                    case sf::Keyboard::Scan::O:
                        current_mode = MODE_OPENMP;
                        break;
                    case sf::Keyboard::Scan::C:
                        current_mode = MODE_CUDA;
                        break;
                    case sf::Keyboard::Scan::Escape:
                        running = 0;
                        window.close();
                        break;
                    default:
                        break;
                }
                std::memset(pixel_buffer, 0, WIDTH * delta * sizeof(uint32_t));
            }
        }

        std::array<int, 3> state = {max_iteraciones, running, current_mode};
        std::array<double, 4> plane = {x_min, y_min, x_max, y_max};

        MPI_Bcast(state.data(), 3, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(plane.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        max_iteraciones = state[0];
        running = state[1];
        current_mode = state[2];
        x_min = plane[0];
        y_min = plane[1];
        x_max = plane[2];
        y_max = plane[3];

        if (running == 0) {
            break;
        }

        double t0 = MPI_Wtime();

        if (current_mode == MODE_CUDA) {
            newton_gpu(max_iteraciones, x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end,
                       device_pixels, d_iters, pixel_buffer, local_iters);
        } else {
            newton_cpu(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end,
                       pixel_buffer, local_iters, (current_mode == MODE_OPENMP));
        }

        local_time_ms = (MPI_Wtime() - t0) * 1000.0;

        MPI_Reduce(&local_time_ms, &max_time_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_iters, &total_iters, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

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

        const char* mode_name = (current_mode == MODE_CUDA) ? "CUDA" : 
                                (current_mode == MODE_OPENMP) ? "OpenMP" : "Serial";

        auto msg = fmt::format("Ranks: {} | Mode: {} | Iter: {} | Max Time: {:.2f} ms | Total Iters: {} | FPS: {}",
            nprocs, mode_name, max_iteraciones, max_time_ms, total_iters, fps);
        text.setString(msg);

        window.clear();
        window.draw(sprite);
        window.draw(text);
        window.draw(textOptions);
        window.display();
    }

    running = 0;
    std::array<int, 3> state = {max_iteraciones, running, current_mode};
    std::array<double, 4> plane = {x_min, y_min, x_max, y_max};

    MPI_Bcast(state.data(), 3, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(plane.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

int main(int argc, char** argv)
{
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

    // Inicializar dispositivo CUDA
    int dev_count = 0;
    cudaGetDeviceCount(&dev_count);
    if (dev_count > 0) {
        cudaSetDevice(rank % dev_count);
        // Reservar memoria en GPU para la subseccion
        size_t gpu_buffer_size = WIDTH * delta * sizeof(uint32_t);
        cudaMalloc(&device_pixels, gpu_buffer_size);
        cudaMalloc(&d_iters, sizeof(unsigned long long));
        copiar_paleta(color_ramp.data());
    }

    fmt::println("RANK: {}, rows: {} to {}, CUDA Devs: {}", rank, row_start, row_end, dev_count);

    if (rank == 0) {
        setup_iu();
    } else {
        while (true) {
            std::array<int, 3> state;
            std::array<double, 4> plane;

            MPI_Bcast(state.data(), 3, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(plane.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            max_iteraciones = state[0];
            running = state[1];
            current_mode = state[2];
            x_min = plane[0];
            y_min = plane[1];
            x_max = plane[2];
            y_max = plane[3];

            if (running == 0) {
                break;
            }

            double t0 = MPI_Wtime();

            if (current_mode == MODE_CUDA) {
                newton_gpu(max_iteraciones, x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end,
                           device_pixels, d_iters, pixel_buffer, local_iters);
            } else {
                newton_cpu(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end,
                           pixel_buffer, local_iters, (current_mode == MODE_OPENMP));
            }

            local_time_ms = (MPI_Wtime() - t0) * 1000.0;

            MPI_Reduce(&local_time_ms, &max_time_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            MPI_Reduce(&local_iters, &total_iters, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

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

    if (device_pixels) cudaFree(device_pixels);
    if (d_iters) cudaFree(d_iters);

    delete[] pixel_buffer;
    if (rank == 0) delete[] texture_buffer;

    MPI_Finalize();
    return 0;
}
