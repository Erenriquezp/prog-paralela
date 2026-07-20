#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include "calor_mpi.h"
#include "palette.h"
#include "mpi.h"
#include "arial_ttf.h"

#ifdef _WIN32
#include <minwindef.h>
#include <windows.h>
#endif

std::string machine_name()
{
    std::string mname = "";
#ifdef _WIN32
    char hostname[256];
    DWORD size = sizeof(hostname);
    GetComputerNameA(hostname, &size);
    mname = hostname;
#else
    mname = "Nodo_Linux";
#endif
    return mname;
}

// Parámetros por defecto de la ecuación de calor 2D
int Nx = 1024;               // Columnas
int Ny = 1024;               // Filas
float alfa = 0.25f;          // Difusividad térmica
float paso_tiempo = 5.0e-7f; // Paso de tiempo
float longitud_x = 1.0f;
float paso_espacial = longitud_x / Nx;
float paso_espacial_cuadrado = paso_espacial * paso_espacial;
double valor_borde_top = 100.0; // Temperatura del borde superior
int max_iteraciones = 1000;

#define WIDTH 1600
#define HEIGHT 900

uint32_t *pixel_buffer = nullptr;   // Buffer de píxeles local de cada proceso 
uint32_t *texture_buffer = nullptr; // Buffer de textura global del Rank 0

int running = 1;
int nproc, rank;
int delta;
int row_start;
int row_end;
int padding;

int iteracion_actual = 0;
double residuo_global_l2 = 1.0;

// Intercambio de celdas fantasma entre procesos vecinos por fila
void intercambiar_fronteras_mpi(float *u, uint32_t columnas_x, uint32_t filas_y_local, int rank, int nproc)
{
    float *enviar_arriba = &u[columnas_x * 1];
    float *recibir_arriba = &u[columnas_x * 0];
    float *enviar_abajo = &u[columnas_x * filas_y_local];
    float *recibir_abajo = &u[columnas_x * (filas_y_local + 1)];

    int vecino_superior = (rank > 0) ? rank - 1 : MPI_PROC_NULL;
    int vecino_inferior = (rank < nproc - 1) ? rank + 1 : MPI_PROC_NULL;

    MPI_Sendrecv(enviar_arriba, columnas_x, MPI_FLOAT, vecino_superior, 0,
                 recibir_arriba, columnas_x, MPI_FLOAT, vecino_superior, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Sendrecv(enviar_abajo, columnas_x, MPI_FLOAT, vecino_inferior, 1,
                 recibir_abajo, columnas_x, MPI_FLOAT, vecino_inferior, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

// Inicializa las grillas locales incluyendo filas fantasma (fila 0 y fila filas_y_local + 1)
void inicializar_grillas_local(float *u_antiguo, float *u_nuevo, uint32_t columnas_x, uint32_t filas_y_local, int rank)
{
    std::fill(u_antiguo, u_antiguo + columnas_x * (filas_y_local + 2), 0.0f);
    std::fill(u_nuevo, u_nuevo + columnas_x * (filas_y_local + 2), 0.0f);

    // Si rank es 0, la primera fila activa (j = 1) es el borde superior caliente (100.0f)
    if (rank == 0)
    {
        for (uint32_t i = 0; i < columnas_x; ++i)
        {
            u_antiguo[columnas_x * 1 + i] = 100.0f;
            u_nuevo[columnas_x * 1 + i] = 100.0f;
        }
    }
}

void setup_ui()
{
    texture_buffer = new uint32_t[WIDTH * HEIGHT];
    std::memset(texture_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));

    // Búfer intermedio donde el Rank 0 reconstruye la grilla física total (1024x1024)
    uint32_t *canvas_reconstruido = new uint32_t[Nx * Ny];
    std::memset(canvas_reconstruido, 0, Nx * Ny * sizeof(uint32_t));

    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Ecuacion de Calor 2D MPI - UCE");

    sf::Texture texture({WIDTH, HEIGHT});
    texture.update((const uint8_t *)texture_buffer);
    sf::Sprite sprite(texture);

    sf::Font font(arial_ttf::data, arial_ttf::data_len);
    sf::Text text(font, "", 20);
    text.setFillColor(sf::Color::White);
    text.setPosition({10, 10});
    text.setStyle(sf::Text::Bold);

    sf::Text textoptions(font, "Teclado: UP/DOWN cambia iteraciones de 50 en 50. Esc para Salir.", 20);
    textoptions.setStyle(sf::Text::Bold);
    textoptions.setPosition({10, window.getView().getSize().y - 40});
    textoptions.setFillColor(sf::Color::Yellow);

    // Inicializar el dominio local para el Rank 0
    float *u_antiguo_local = new float[Nx * (delta + 2)];
    float *u_nuevo_local = new float[Nx * (delta + 2)];
    inicializar_grillas_local(u_antiguo_local, u_nuevo_local, Nx, delta, rank);

    int iteracion_objetivo = 50;

    int frame = 0;
    int fps = 0;
    sf::Clock clock;
    double mflops = 0.0;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                running = 0;
                window.close();
            }
            else if (event->is<sf::Event::KeyReleased>())
            {
                auto evt = event->getIf<sf::Event::KeyReleased>();
                switch (evt->scancode)
                {
                case sf::Keyboard::Scan::Up:
                    iteracion_objetivo = std::min(max_iteraciones, iteracion_objetivo + 50);
                    break;

                case sf::Keyboard::Scan::Down:
                    iteracion_objetivo = std::max(10, iteracion_objetivo - 50);
                    break;

                case sf::Keyboard::Scan::Escape:
                    running = 0;
                    window.close();
                    break;
                }
            }
        }

        // Sincronizar el objetivo seleccionado y el estado de ejecución con los esclavos
        std::vector<int> dummy = {iteracion_objetivo, running};
        MPI_Bcast(dummy.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);

        if (running == 0)
        {
            break;
        }

        // 1. Si se baja el valor con DOWN, reiniciamos a frío para poder retroceder
        if (iteracion_actual > iteracion_objetivo)
        {
            inicializar_grillas_local(u_antiguo_local, u_nuevo_local, Nx, delta, rank);
            iteracion_actual = 0;
            residuo_global_l2 = 1.0;
        }

        // 2. Calculamos los pasos para llegar a la iteración objetivo
        auto t_start = std::chrono::high_resolution_clock::now();
        int pasos_ejecutados = 0;
        while (iteracion_actual < iteracion_objetivo)
        {
            if (residuo_global_l2 < 1.0e-4)
            {
                break;
            }

            intercambiar_fronteras_mpi(u_antiguo_local, Nx, delta, rank, nproc);

            double suma_diferencias_local = 0.0;
            calor_mpi(u_antiguo_local, u_nuevo_local, Nx, delta,
                      alfa, paso_tiempo, paso_espacial_cuadrado,
                      pixel_buffer, suma_diferencias_local);

            // Reducir la suma de diferencias para obtener el residuo L2 global
            double suma_diferencias_global = 0.0;
            MPI_Allreduce(&suma_diferencias_local, &suma_diferencias_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            residuo_global_l2 = std::sqrt(suma_diferencias_global / (Nx * Ny));

            std::swap(u_antiguo_local, u_nuevo_local);
            iteracion_actual++;
            pasos_ejecutados++;
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        double t_elapsed = std::chrono::duration<double>(t_end - t_start).count();

        if (pasos_ejecutados > 0 && t_elapsed > 0.0)
        {
            double ops = (double)(Nx - 2) * (Ny - 2) * 7.0 * pasos_ejecutados;
            mflops = ops / (t_elapsed * 1e6);
        }

        // Copiar el pedazo procesado por el Rank 0 al canvas global
        std::memcpy(canvas_reconstruido, pixel_buffer, Nx * delta * sizeof(uint32_t));

        // buffer temporal para recibir de los esclavos
        uint32_t *recv_buffer = new uint32_t[Nx * delta];

        // 3. Recibir las porciones calculadas por los procesos esclavos
        for (int i = 1; i < nproc; i++)
        {
            int new_delta = delta;
            if (i == nproc - 1)
            {
                new_delta = delta - padding;
            }
            MPI_Status status;
            MPI_Recv(
                recv_buffer,
                Nx * new_delta,
                MPI_UINT32_T,
                i,
                0,
                MPI_COMM_WORLD,
                &status);

            // Reconstruir la matriz global uniendo las franjas recibidas
            std::memcpy(canvas_reconstruido + i * delta * Nx, recv_buffer, Nx * new_delta * sizeof(uint32_t));
        }

        delete[] recv_buffer; // Liberar el buffer temporal en cada frame

        // 4. Mapear y estirar la grilla (1024x1024) al buffer de la ventana de SFML (1600x900)
        double ratio_x = static_cast<double>(Nx - 1) / WIDTH;
        double ratio_y = static_cast<double>(Ny - 1) / HEIGHT;

        for (int y = 0; y < HEIGHT; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                int gx = std::clamp(static_cast<int>(x * ratio_x), 0, Nx - 1);
                int gy = std::clamp(static_cast<int>(y * ratio_y), 0, Ny - 1);

                texture_buffer[y * WIDTH + x] = canvas_reconstruido[gy * Nx + gx];
            }
        }

        texture.update((const uint8_t *)texture_buffer);

        // Control de FPS
        frame++;
        if (clock.getElapsedTime().asSeconds() >= 1.0f)
        {
            fps = frame;
            frame = 0;
            clock.restart();
        }

        // Renderizar textos
        std::string estado_convergencia = (residuo_global_l2 < 1.0e-4) ? " (CONVERGIDO)" : "";
        auto msg = fmt::format(
            "BACKEND ACTIVO: MPI (Distribuido)\n"
            "RANK: {} - {}\n"
            "Iteracion: {} / {}{}\n"
            "Residuo L2: {:.6f}\n"
            "MFLOPS: {:.2f}\n"
            "FPS: {}",
            rank, machine_name(), iteracion_actual, max_iteraciones, estado_convergencia, residuo_global_l2, mflops, fps);
        text.setString(msg);

        window.clear();
        {
            window.draw(sprite);
            window.draw(text);
            window.draw(textoptions);
        }
        window.display();
    }

    delete[] u_antiguo_local;
    delete[] u_nuevo_local;
    delete[] canvas_reconstruido;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Descomposición del dominio de trabajo por filas de grilla
    delta = std::ceil(Ny * 1.0 / nproc);
    row_start = rank * delta;
    row_end = std::min(row_start + delta, Ny);
    padding = delta * nproc - Ny;

    if (row_end > Ny)
    {
        row_end = Ny;
    }

    // Reservar memoria del buffer de píxeles local
    pixel_buffer = new uint32_t[Nx * delta];
    std::memset(pixel_buffer, 0, Nx * delta * sizeof(uint32_t));

    if (rank == 0)
    {
        setup_ui();
    }
    else
    {
        int Ny_local = row_end - row_start;
        float *u_antiguo_local = new float[Nx * (Ny_local + 2)];
        float *u_nuevo_local = new float[Nx * (Ny_local + 2)];
        inicializar_grillas_local(u_antiguo_local, u_nuevo_local, Nx, Ny_local, rank);

        int iteracion_actual_esclavo = 0;

        while (true)
        {
            // Sincronizar el objetivo manual recibido desde el Rank 0
            std::vector<int> dummy = {0, 0};
            MPI_Bcast(dummy.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
            int iteracion_objetivo_esclavo = dummy[0];
            running = dummy[1];

            if (running == 0)
            {
                break;
            }

            // Si el objetivo bajó con DOWN, el esclavoo también se reinicia a frío
            if (iteracion_actual_esclavo > iteracion_objetivo_esclavo)
            {
                inicializar_grillas_local(u_antiguo_local, u_nuevo_local, Nx, Ny_local, rank);
                iteracion_actual_esclavo = 0;
                residuo_global_l2 = 1.0;
            }

            // El esclavo procesa hasta alcanzar la iteración seleccionada
            while (iteracion_actual_esclavo < iteracion_objetivo_esclavo)
            {
                if (residuo_global_l2 < 1.0e-4)
                {
                    break;
                }

                // Intercambiar filas fantasma
                intercambiar_fronteras_mpi(u_antiguo_local, Nx, Ny_local, rank, nproc);

                double suma_diferencias_local = 0.0;
                calor_mpi(u_antiguo_local, u_nuevo_local, Nx, Ny_local,
                          alfa, paso_tiempo, paso_espacial_cuadrado,
                          pixel_buffer, suma_diferencias_local);

                // Reducir la suma de diferencias para obtener el residuo L2 global
                double suma_diferencias_global = 0.0;
                MPI_Allreduce(&suma_diferencias_local, &suma_diferencias_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
                residuo_global_l2 = std::sqrt(suma_diferencias_global / (Nx * Ny));

                std::swap(u_antiguo_local, u_nuevo_local);
                iteracion_actual_esclavo++;
            }

            // Envío punto a punto de la porción de píxeles calculada al Rank 0
            int send_rows = Ny_local;
            MPI_Send(
                pixel_buffer,
                Nx * send_rows,
                MPI_UINT32_T,
                0,
                0,
                MPI_COMM_WORLD);
        }

        delete[] u_antiguo_local;
        delete[] u_nuevo_local;
    }

    delete[] pixel_buffer;
    if (rank == 0)
    {
        delete[] texture_buffer;
    }

    MPI_Finalize();
    return 0;
}