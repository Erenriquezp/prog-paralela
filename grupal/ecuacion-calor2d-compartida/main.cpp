#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>

#include "calor_serial.h"
#include "calor_simd.h"
#include "calor_openmp.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Dimensiones de la Ventana SFML
#define ANCHO 1600
#define ALTO 900

// Dimensiones de la Grilla Física
const uint32_t columnas_x = 1024;
const uint32_t filas_y = 1024;

// Parámetros de la Ecuación de Calor
const float alfa = 0.25f;
const float paso_tiempo = 5.0e-7f;
const float longitud_x = 1.0f;
const float longitud_y = 1.0f;
const float paso_espacial = longitud_x / columnas_x;
const float paso_espacial_cuadrado = paso_espacial * paso_espacial;

// Buffers de temperatura
float* u_antiguo = nullptr;
float* u_nuevo = nullptr;
uint32_t* buffer_pixeles = nullptr;

enum class tipo_ejecucion
{
    SERIAL = 0,
    SIMD,
    OpenMP_REGIONES
};

// Inicializa las condiciones de contorno
void inicializar_grillas()
{
    for (uint32_t j = 0; j < filas_y; ++j)
    {
        for (uint32_t i = 0; i < columnas_x; ++i)
        {
            uint32_t indice = j * columnas_x + i;
            if (j == 0)
            {
                u_antiguo[indice] = 100.0f;
                u_nuevo[indice] = 100.0f;
            }
            else
            {
                u_antiguo[indice] = 0.0f;
                u_nuevo[indice] = 0.0f;
            }
        }
    }
}

int main()
{
    int maximo_hilos = 1;
#pragma omp parallel
    {
#pragma omp master
        {
            maximo_hilos = omp_get_num_threads();
        }
    }

    tipo_ejecucion modo_ejecucion = tipo_ejecucion::SERIAL;
    bool paso_adelante = false;
    bool paso_atras = false;
    bool recalcular_ultimo_paso = false;
    uint32_t contador_iteraciones = 0;
    double residuo = 0.0;
    double mflops = 0.0;
    bool convergido = false;

    // Reservar memoria
    u_antiguo = new float[columnas_x * filas_y];
    u_nuevo = new float[columnas_x * filas_y];
    buffer_pixeles = new uint32_t[columnas_x * filas_y];

    inicializar_grillas();

    // Mapear primer estado a buffer_pixeles inicialmente
    double residuo_inicial = 0.0;
    calor_serial(u_antiguo, u_nuevo, columnas_x, filas_y, alfa, paso_tiempo, paso_espacial_cuadrado, buffer_pixeles, residuo_inicial);

    std::vector<std::vector<float>> historial_u;
    historial_u.push_back(std::vector<float>(u_antiguo, u_antiguo + columnas_x * filas_y));

    sf::RenderWindow ventana(sf::VideoMode({ANCHO, ALTO}), "Ecuacion de Calor 2D - UCE");

#ifdef _WIN32
    HWND hwnd = ventana.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif

    sf::Texture textura({columnas_x, filas_y});
    sf::Sprite sprite(textura);

    // Ajustar escala de la textura al tamaño de la ventana
    sprite.setScale({ (float)ANCHO / columnas_x, (float)ALTO / filas_y });

    sf::Font fuente;
    if (!fuente.openFromFile("arial.ttf") && !fuente.openFromFile("build/Debug/arial.ttf"))
    {
        std::cerr << "Error al cargar la fuente arial.ttf" << std::endl;
    }

    // Textos de diagnóstico
    sf::Text texto_diagnostico(fuente, "", 20);
    texto_diagnostico.setFillColor(sf::Color::White);
    texto_diagnostico.setPosition({10, 10});
    texto_diagnostico.setStyle(sf::Text::Bold);

    std::string opciones = "Controles: [1] Serial | [2] SIMD (AVX2) | [3] OpenMP Regiones | UP: Adelante | DOWN: Atras | Esc para Salir.";
    sf::Text texto_opciones(fuente, opciones, 20);
    texto_opciones.setFillColor(sf::Color::Yellow);
    texto_opciones.setStyle(sf::Text::Bold);
    
    texto_opciones.setPosition({10, ventana.getView().getSize().y - 40});

    int fps = 0;

    while (ventana.isOpen())
    {
        // 1. Manejo de Eventos
        while (const std::optional event = ventana.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                ventana.close();
            else if (event->is<sf::Event::KeyReleased>())
            {
                auto evt = event->getIf<sf::Event::KeyReleased>();
                switch (evt->scancode)
                {
                case sf::Keyboard::Scan::Num1:
                    modo_ejecucion = tipo_ejecucion::SERIAL;
                    recalcular_ultimo_paso = true;
                    break;
                case sf::Keyboard::Scan::Num2:
                    modo_ejecucion = tipo_ejecucion::SIMD;
                    recalcular_ultimo_paso = true;
                    break;
                case sf::Keyboard::Scan::Num3:
                    modo_ejecucion = tipo_ejecucion::OpenMP_REGIONES;
                    recalcular_ultimo_paso = true;
                    break;
                case sf::Keyboard::Scan::Up:
                    paso_adelante = true;
                    break;
                case sf::Keyboard::Scan::Down:
                    paso_atras = true;
                    break;
                case sf::Keyboard::Scan::Escape:
                    ventana.close();
                    break;
                default:
                    break;
                }
            }
        }

        // 2. Ejecutar Cómputo si corresponde
        std::string modo = "";

        if (recalcular_ultimo_paso)
        {
            if (contador_iteraciones >= 50)
            {
                uint32_t proxima_iter = contador_iteraciones - 50;
                uint32_t indice_historial = proxima_iter / 50;
                if (indice_historial < historial_u.size())
                {
                    std::copy(historial_u[indice_historial].begin(), historial_u[indice_historial].end(), u_antiguo);
                    contador_iteraciones = proxima_iter;
                    convergido = false;
                    paso_adelante = true;
                }
            }
            recalcular_ultimo_paso = false;
        }

        if (paso_atras)
        {
            if (contador_iteraciones >= 50)
            {
                uint32_t proxima_iter = contador_iteraciones - 50;
                uint32_t indice_historial = proxima_iter / 50;
                if (indice_historial < historial_u.size())
                {
                    std::copy(historial_u[indice_historial].begin(), historial_u[indice_historial].end(), u_antiguo);
                    contador_iteraciones = proxima_iter;
                    convergido = false;

                    // Actualizar buffer de píxeles y residuo
                    double residuo_ficticio = 0.0;
                    calor_serial(u_antiguo, u_nuevo, columnas_x, filas_y, alfa, paso_tiempo, paso_espacial_cuadrado, buffer_pixeles, residuo_ficticio);
                    residuo = residuo_ficticio;
                }
            }
            paso_atras = false;
        }
        else if (paso_adelante)
        {
            double residuo_paso = 0.0;
            auto tiempo_inicio = std::chrono::high_resolution_clock::now();

            int pasos_ejecutados = 0;
            for (int paso = 0; paso < 50; ++paso)
            {
                if (contador_iteraciones >= 1000 || convergido)
                    break;

                if (modo_ejecucion == tipo_ejecucion::SERIAL)
                {
                    calor_serial(u_antiguo, u_nuevo, columnas_x, filas_y, alfa, paso_tiempo, paso_espacial_cuadrado, buffer_pixeles, residuo_paso);
                }
                else if (modo_ejecucion == tipo_ejecucion::SIMD)
                {
                    calor_simd(u_antiguo, u_nuevo, columnas_x, filas_y, alfa, paso_tiempo, paso_espacial_cuadrado, buffer_pixeles, residuo_paso);
                }
                else if (modo_ejecucion == tipo_ejecucion::OpenMP_REGIONES)
                {
                    calor_openmp_regiones(u_antiguo, u_nuevo, columnas_x, filas_y, alfa, paso_tiempo, paso_espacial_cuadrado, buffer_pixeles, residuo_paso);
                }

                // Intercambiar buffers
                std::swap(u_antiguo, u_nuevo);

                residuo = residuo_paso;
                contador_iteraciones++;
                pasos_ejecutados++;

                if (residuo < 1.0e-4)
                {
                    convergido = true;
                }
            }

            auto tiempo_fin = std::chrono::high_resolution_clock::now();
            double tiempo_transcurrido = std::chrono::duration<double>(tiempo_fin - tiempo_inicio).count();

            // Calcular MFLOPS para el lote
            double operaciones_flotantes = (double)(columnas_x - 2) * (filas_y - 2) * 7.0 * pasos_ejecutados;
            mflops = (tiempo_transcurrido > 0.0) ? (operaciones_flotantes / (tiempo_transcurrido * 1e6)) : 0.0;

            fps = (tiempo_transcurrido > 0.0) ? (int)(pasos_ejecutados / tiempo_transcurrido) : 0;

            // Guardar en historial
            uint32_t indice_historial = contador_iteraciones / 50;
            std::vector<float> estado_actual(u_antiguo, u_antiguo + columnas_x * filas_y);
            if (indice_historial < historial_u.size())
            {
                historial_u[indice_historial] = estado_actual;
            }
            else
            {
                historial_u.push_back(estado_actual);
            }

            paso_adelante = false;
        }

        // 3. Obtener nombre del modo actual
        if (modo_ejecucion == tipo_ejecucion::SERIAL) modo = "Serial";
        else if (modo_ejecucion == tipo_ejecucion::SIMD) modo = "SIMD (AVX2)";
        else if (modo_ejecucion == tipo_ejecucion::OpenMP_REGIONES) modo = fmt::format("OpenMP Regiones [Hilos: {}]", maximo_hilos);

        // 4. Cargar el buffer a la GPU
        textura.update((const uint8_t *)buffer_pixeles);

        // 6. Mostrar Estadísticas
        auto msg = fmt::format(
            "BACKEND ACTIVO: {}\n"
            "Iteracion: {} / 1000\n"
            "Residuo L2: {:.6f}\n"
            "MFLOPS: {:.2f}\n"
            "FPS: {}",
            modo, contador_iteraciones, residuo, mflops, fps
        );
        texto_diagnostico.setString(msg);

        // 7. Renderizar Elementos
        ventana.clear();
        ventana.draw(sprite);
        ventana.draw(texto_diagnostico);
        ventana.draw(texto_opciones);
        ventana.display();
    }

    // Liberar memoria
    delete[] u_antiguo;
    delete[] u_nuevo;
    delete[] buffer_pixeles;

    return 0;
}