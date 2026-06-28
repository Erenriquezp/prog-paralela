# PLANTILLAS DE EJERCICIOS

Este archivo contiene plantillas reutilizables para ejercicios prácticos relacionados con el proyecto. Cada sección incluye un título descriptivo y una breve explicación para facilitar su comprensión y uso.

## Plantilla 1: Configuración básica de SFML en C++

Esta plantilla muestra cómo configurar una ventana básica utilizando SFML en C++.

```cpp
#include <SFML/Graphics.hpp>

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Ventana SFML");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.display();
    }

    return 0;
}
```

### Explicación:

- `sf::RenderWindow`: Crea una ventana gráfica.
- `window.clear()` y `window.display()`: Limpian y actualizan la ventana en cada frame.

---

## Plantilla 2: Uso de SIMD con AVX2 en C++

Esta plantilla demuestra cómo realizar operaciones vectorizadas utilizando intrínsecos AVX2.

```cpp
#include <immintrin.h>
#include <iostream>

int main() {
    __m256d a = _mm256_set1_pd(1.0); // Vector con cuatro elementos de valor 1.0
    __m256d b = _mm256_set1_pd(2.0); // Vector con cuatro elementos de valor 2.0
    __m256d result = _mm256_add_pd(a, b); // Suma vectorizada

    double* res = (double*)&result;
    for (int i = 0; i < 4; ++i) {
        std::cout << "Resultado[" << i << "]: " << res[i] << std::endl;
    }

    return 0;
}
```

### Explicación:

- `__m256d`: Tipo de dato para vectores de 256 bits (4 doubles).
- `_mm256_add_pd`: Realiza una suma vectorizada de doubles.

---

## Plantilla 3: Renderizado básico con OpenGL en Java

Esta plantilla ilustra cómo inicializar una ventana y un contexto OpenGL utilizando LWJGL en Java.

```java
import org.lwjgl.glfw.GLFW;
import org.lwjgl.glfw.GLFWErrorCallback;
import org.lwjgl.opengl.GL;

public class OpenGLExample {
    public static void main(String[] args) {
        GLFWErrorCallback.createPrint(System.err).set();

        if (!GLFW.glfwInit()) {
            throw new IllegalStateException("No se pudo inicializar GLFW");
        }

        long window = GLFW.glfwCreateWindow(800, 600, "Ventana OpenGL", 0, 0);
        if (window == 0) {
            throw new RuntimeException("No se pudo crear la ventana GLFW");
        }

        GLFW.glfwMakeContextCurrent(window);
        GL.createCapabilities();

        while (!GLFW.glfwWindowShouldClose(window)) {
            GLFW.glfwPollEvents();
            GLFW.glfwSwapBuffers(window);
        }

        GLFW.glfwDestroyWindow(window);
        GLFW.glfwTerminate();
    }
}
```

### Explicación:

- `GLFW.glfwInit()`: Inicializa la biblioteca GLFW.
- `GL.createCapabilities()`: Configura el contexto OpenGL.
- `GLFW.glfwSwapBuffers(window)`: Intercambia los buffers de la ventana para mostrar el contenido renderizado.

---

## Plantilla 4: Carga de una DLL en Java con JNR-FFI

Esta plantilla muestra cómo cargar y utilizar una DLL nativa desde Java utilizando JNR-FFI.

```java
import jnr.ffi.LibraryLoader;

public class DllExample {
    public interface NativeLibrary {
        void sayHello();
    }

    public static void main(String[] args) {
        NativeLibrary lib = LibraryLoader.create(NativeLibrary.class).load("mi_dll");
        lib.sayHello();
    }
}
```

### Explicación:

- `LibraryLoader.create()`: Carga la biblioteca nativa especificada.
- `sayHello()`: Método definido en la DLL que se invoca desde Java.

---

Estas plantillas están diseñadas para ser reutilizadas y adaptadas según las necesidades de los ejercicios y proyectos del usuario.
