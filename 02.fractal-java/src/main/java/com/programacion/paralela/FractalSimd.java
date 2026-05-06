package com.programacion.paralela;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class FractalSimd {
    ByteBuffer pixelBuffer;

    public FractalSimd() {
        // native order para que coincida con lo que escribe la DLL (little-endian en Windows)
        this.pixelBuffer = ByteBuffer.allocateDirect(FractalParams.WIDTH * FractalParams.HEIGHT * Integer.BYTES)
                                     .order(ByteOrder.nativeOrder());
    }

    public void juliaSimd() {
        FractalDll.INSTANCE.julia_simd(
            FractalParams.xMin, FractalParams.yMin, FractalParams.xMax, FractalParams.yMax,
            FractalParams.WIDTH, FractalParams.HEIGHT,
            FractalParams.maxIteraciones,
            pixelBuffer
        );
        pixelBuffer.rewind();
    }
}
