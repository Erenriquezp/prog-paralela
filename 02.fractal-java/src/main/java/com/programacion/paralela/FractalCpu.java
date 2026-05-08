package com.programacion.paralela;

public class FractalCpu {

    public int[] pixelBuffer;

    public FractalCpu() {
        pixelBuffer = new int[FractalParams.WIDTH * FractalParams.HEIGHT];
    }

    int acotado_2(double x, double y) {

        int iter = 1;
        double zr = x;
        double zi = y;

        while (iter<FractalParams.maxIteraciones && (zr*zr + zi*zi)<4.0) {

            double dr = zr*zr - zi*zi + FractalParams.cReal;
            double di = 2.0*zr*zi + FractalParams.cImag;
            zr = dr;
            zi = di;

            iter++;
        }

        if (iter<FractalParams.maxIteraciones) {
            // la norma es mayor que 2
            int index = iter % FractalParams.PALETTE_SIZE;
            return FractalParams.colorRamp[index];
        }

        return 0xFF000000; // color negreo


    }

    int acotado_threads_2(double x, double y) {

        int iter = 1;
        double zr = x;
        double zi = y;

        while (iter<FractalParams.maxIteraciones && (zr*zr + zi*zi)<4.0) {

            double dr = zr*zr - zi*zi + FractalParams.cReal;
            double di = 2.0*zr*zi + FractalParams.cImag;
            zr = dr;
            zi = di;

            iter++;
        }

        if (iter<FractalParams.maxIteraciones) {
            int index = iter % FractalParams.PALETTE_SIZE;
            return FractalParams.colorRampThreads[index];
        }

        return 0xFF000000;
    }

    void julia_serial_2(double x_min, double y_min, double x_max, double y_max, int width, int height) {
        double dx = (x_max - x_min) / width;
        double dy = (y_max - y_min) / height;

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                // z = x+yi - (x, y)
                double x = x_min + i * dx;
                double y = y_max - j * dy;

                var color = acotado_2(x, y);

                pixelBuffer[j * width + i] = color; // La textura es unidimensional
            }

        }
    }

    void julia_threads_2(double x_min, double y_min, double x_max, double y_max, int width, int height) {
        double dx = (x_max - x_min) / width;
        double dy = (y_max - y_min) / height;

        // Cantidad de hilos segun procesadores disponibles en la maquina.
        int cores = Runtime.getRuntime().availableProcessors();
        // No tiene sentido usar mas hilos que filas de trabajo.
        int threadCount = Math.min(cores, height);
        Thread[] threads = new Thread[threadCount];

        // Reparto de filas por hilo (bloques contiguos).
        int rowsPerThread = height / threadCount;
        int remainder = height % threadCount;
        int startRow = 0;

        for (int t = 0; t < threadCount; t++) {
            int extra = (t < remainder) ? 1 : 0;
            int endRow = startRow + rowsPerThread + extra;
            int rowStart = startRow;

            threads[t] = new Thread(() -> {
                // Cada hilo calcula solo su rango de filas.
                for (int j = rowStart; j < endRow; j++) {
                    for (int i = 0; i < width; i++) {
                        double x = x_min + i * dx;
                        double y = y_max - j * dy;
                        int color = acotado_threads_2(x, y);
                        pixelBuffer[j * width + i] = color;
                    }
                }
            });

            threads[t].start();
            startRow = endRow;
        }

        // Espera a que todos los hilos terminen antes de pintar.
        for (Thread thread : threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                throw new RuntimeException("Error al esperar threads del fractal", e);
            }
        }
    }
}
