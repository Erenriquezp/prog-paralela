## 🛠️ Herramientas y Versiones
* **Compilador:** Winlibs GCC 15.2.0 (POSIX threads + UCRT).
https://github.com/brechtsanders/winlibs_mingw/releases
winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-14.0.0-r7.7z
102 MB 2 weeks ago
* **Gestor de Librerías:** vcpkg (Versión 2026.03.18).
https://github.com/microsoft/vcpkg/releases/tag/2026.03.18

* **Build System:** CMake + Ninja Multi-Config.

* **Zoom:** 550 558 9245
* **PATH IntelliJ:** C:\tools\mingw64\bin;C:\tools\prog-paralela\03.fractal-dll\build\Release
---

## 💻 Comandos Esenciales (Consola)

### 1. Preparación del Compilador
```powershell
# Temporal: Añadir al PATH en la sesión actual
set PATH=c:/tools/mingw64/bin;%PATH%

# Verificación de herramientas
gcc --version
g++ --version
ninja --version
```

### 2. Configuración de vcpkg
```powershell
# Inicializar el gestor (solo la primera vez) en C:\tools\vcpkg-2026.03.18>
.\bootstrap-vcpkg.bat

# Instalar dependencias del curso (Triplet x64-mingw-dynamic)
.\vcpkg install fmt:x64-mingw-dynamic
.\vcpkg install sfml:x64-mingw-dynamic
```
### 3. OPEN MPI

```powershell
cd C:\oneAPI
C:\oneAPI>setvars
C:\tools\prog-paralela\05.ejemplo-mpi\build\Debug>mpiexec -n 4 fractal-mpi.exe
```
---

## 📂 Archivos del Proyecto (Explicación)

| Archivo | Explicación (1-2 líneas) |
| :--- | :--- |
| **`main.cpp`** | Punto de entrada del programa; contiene la lógica paralela y el bucle de renderizado SFML 3. |
| **`CMakeLists.txt`** | Script principal que define cómo compilar el proyecto y dónde encontrar las librerías (`find_package`). |
| **`vcpkg.json`** | Modo manifiesto; lista las dependencias para que vcpkg las instale automáticamente al configurar. |
| **`settings.json`** | Configuración local de VS Code para enlazar el motor de CMake con tus herramientas en `C:/tools`. |

---

## 🧩 Extensiones de VS Code
* **C/C++ Extension Pack:** Provee IntelliSense, depuración y soporte de lenguaje para C++20.
* **CMake Tools:** Gestiona la configuración, compilación y ejecución mediante el archivo `CMakeLists.txt`.
* **Error Lens:** Resalta errores y advertencias directamente sobre la línea de código para corrección rápida.

---

## ⚙️ Rutas del Sistema
* **Binarios de herramientas:** `C:\tools\mingw64\bin`
* **Toolchain de vcpkg:** `C:\tools\vcpkg-2026.03.18\scripts\buildsystems\vcpkg.cmake`
* **Directorio de trabajo:** `C:\tools\prog-paralela\ejemplo01`

---

## 💡 Recordatorio de Sintaxis SFML 3 (Unidad 1)
Debido a que esta versión es la que instalamos, recuerda estos cambios críticos:
* **VideoMode:** Usa llaves: `sf::VideoMode({ancho, alto})`.
* **Eventos:** Usa opcionales: `while (const std::optional event = window.pollEvent())`.
* **Cierre:** Verifica tipo: `if (event->is<sf::Event::Closed>())`.
