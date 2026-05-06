package com.programacion.paralela;

public class FractalParams {

    public static final int WIDTH = 1600;
    public static final int HEIGHT = 900;

    public static int maxIteraciones = 10;

    public static final double xMin = -1.5;
    public  static final double xMax = 1.5;
    public static final double yMin = -1.0;
    public  static final double yMax = 1.0;

    public static final double cReal = -0.7;
    public static final double cImag = 0.27015;
    public  static final int PALETTE_SIZE = 16;

   // Paleta azul -> negro (modo Java CPU)
   // Formato int little-endian para GL_RGBA GL_UNSIGNED_BYTE: byte[0]=R, byte[1]=G, byte[2]=B, byte[3]=A
   // Azul puro = R=0x00, G=0x00, B=0xFF, A=0xFF => int = 0xFFFF0000
   public static final int colorRamp[] = {
            0xFFFF0000,  // azul puro
            0xFFEE0000,
            0xFFDD0000,
            0xFFCC0000,
            0xFFBB0000,
            0xFFAA0000,
            0xFF990000,
            0xFF880000,
            0xFF770000,
            0xFF660000,
            0xFF550000,
            0xFF440000,
            0xFF330000,
            0xFF220000,
            0xFF110000,
            0xFF080000,  // casi negro
    };
}
