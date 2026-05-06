package com.programacion.paralela;

import org.lwjgl.*;
import org.lwjgl.glfw.*;
import org.lwjgl.opengl.*;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import static org.lwjgl.glfw.Callbacks.*;
import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.system.MemoryUtil.*;

public class FractalMain {

    // window handle
    private long window;
    private int textureID;
    private int overlayTextureID;

    private IntBuffer pixelBuffer;

    FractalCpu fractalCpu = new FractalCpu();
    FractalSimd fractalSimd = new FractalSimd();

    FPSCounter fpsCounter = new FPSCounter();

    int modo = 1; // 1 cpu - 2 simd

    public FractalMain() {
        fractalCpu = new FractalCpu();
        fractalSimd = new FractalSimd();

        pixelBuffer = BufferUtils.createIntBuffer(FractalParams.WIDTH * FractalParams.HEIGHT);
    }

    public void run() {
        System.out.println("Hello Julia " + Version.getVersion() + "!");

        init();
        loop();

        // Free the window callbacks and destroy the window
        glfwFreeCallbacks(window);
        glfwDestroyWindow(window);

        // Terminate GLFW and free the error callback
        glfwTerminate();
        glfwSetErrorCallback(null).free();
    }

    private void init() {
        // Setup an error callback. The default implementation
        // will print the error message in System.err.
        GLFWErrorCallback.createPrint(System.err).set();

        // Initialize GLFW. Most GLFW functions will not work before doing this.
        if ( !glfwInit() )
            throw new IllegalStateException("Unable to initialize GLFW");

        // Configure GLFW
        glfwDefaultWindowHints(); // optional, the current window hints are already the default
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // the window will stay hidden after creation
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // the window will be resizable

        // Create the window
        window = glfwCreateWindow(FractalParams.WIDTH, FractalParams.HEIGHT, "Julia Set!", NULL, NULL);
        if ( window == NULL )
            throw new RuntimeException("Failed to create the GLFW window");

        // Setup a key callback. It will be called every time a key is pressed, repeated or released.
        glfwSetKeyCallback(window, (window, key, scancode, action, mods) -> {
            if ( key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE )
                glfwSetWindowShouldClose(window, true); // We will detect this in the rendering loop
            if (key == GLFW_KEY_UP && action == GLFW_RELEASE )
                FractalParams.maxIteraciones += 10;
            if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE ) {
                FractalParams.maxIteraciones -= 10;
                if (FractalParams.maxIteraciones < 0)
                    FractalParams.maxIteraciones = 10;
            }
            if (key == GLFW_KEY_1 && action == GLFW_RELEASE) {
                System.out.println("Modo Java CPU");
                modo = 1;
            }
            if (key == GLFW_KEY_2 && action == GLFW_RELEASE) {
                System.out.println("Modo C/C++ SIMD");
                modo = 2;
            }
        });

        GLFWVidMode vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(window,
                (vidmode.width()-FractalParams.WIDTH)/2,
                (vidmode.height()-FractalParams.HEIGHT)/2);

                // Make the OpenGL context current
        glfwMakeContextCurrent(window);

        GL.createCapabilities();
        GL.createCapabilitiesWGL();

        // --version OPENGL
        String version = GL11.glGetString(GL11.GL_VERSION);
        String vendor = GL11.glGetString(GL11.GL_VENDOR);
        String renderer = GL11.glGetString(GL11.GL_RENDERER);

        System.out.println("version = " + version);
        System.out.println("vendor = " + vendor);
        System.out.println("renderer = " + renderer);


        //-- conf proyeccion
        GL11.glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1,1,-1,1,-1,1);

        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_TEXTURE_2D);
        glLoadIdentity();

        // Enable v-sync
        glfwSwapInterval(1);

        // Make the window visible
        glfwShowWindow(window);

        setupTexture();
        setupOverlayTexture();
    }

    private void setupTexture() {
        textureID = glGenTextures();
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA8,
                FractalParams.WIDTH, FractalParams.HEIGHT,
                0, GL_RGBA, GL_UNSIGNED_BYTE, NULL
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    private void setupOverlayTexture() {
        overlayTextureID = glGenTextures();
        glBindTexture(GL_TEXTURE_2D, overlayTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    private void loop() {
        // This line is critical for LWJGL's interoperation with GLFW's
        // OpenGL context, or any context that is managed externally.
        // LWJGL detects the context that is current in the current thread,
        // creates the GLCapabilities instance and makes the OpenGL
        // bindings available for use.
        GL.createCapabilities();

        // Set the clear color
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Run the rendering loop until the user has attempted to close
        // the window or has pressed the ESCAPE key.
        while ( !glfwWindowShouldClose(window) ) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the framebuffer


            paint();

            glfwSwapBuffers(window); // swap the color buffers

            // Poll for window events. The key callback above will only be
            // invoked during this call.
            glfwPollEvents();
        }
    }

    private void paint() {

        int fps = fpsCounter.update();
        System.out.println("FPS: " + fps);
        pixelBuffer.clear();

        if (modo == 1) {
            fractalCpu.julia_serial_2(FractalParams.xMin, FractalParams.yMin, FractalParams.xMax, FractalParams.yMax, FractalParams.WIDTH, FractalParams.HEIGHT);
            pixelBuffer.put(fractalCpu.pixelBuffer);
        } else if (modo == 2) {
            fractalSimd.juliaSimd();
            pixelBuffer.put(fractalSimd.pixelBuffer.asIntBuffer());
        }

        pixelBuffer.flip();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA8,
                FractalParams.WIDTH, FractalParams.HEIGHT,
                0, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer
        );

        glBegin(GL_QUADS);
        {
            glTexCoord2f(0, 0);
            glVertex2d(-1, -1);

            glTexCoord2f(0, 1);
            glVertex2d(-1, 1);

            glTexCoord2f(1, 1);
            glVertex2d(1, 1);

            glTexCoord2f(1, 0);
            glVertex2d(1, -1);
        }
        glEnd();

        drawOverlay(fps);
    }

    private void drawOverlay(int fps) {
        BufferedImage image = new BufferedImage(FractalParams.WIDTH, FractalParams.HEIGHT, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = image.createGraphics();
        try {
            g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
            g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

            String modoNombre = (modo == 1) ? "Java CPU" : "C++ SIMD";

            g.setColor(Color.WHITE);
            g.setFont(new Font("SansSerif", Font.BOLD, 24));
            g.drawString("Julia Set | Iteraciones: " + FractalParams.maxIteraciones + " | FPS: " + fps + " | Modo: " + modoNombre, 10, 28);

            g.setFont(new Font("SansSerif", Font.BOLD, 20));
            g.drawString("Opciones: [Up/Down] Cambiar iteraciones | [1] Java CPU | [2] C++ SIMD | [Esc] Salir", 10, FractalParams.HEIGHT - 20);
        } finally {
            g.dispose();
        }

        ByteBuffer buffer = ByteBuffer.allocateDirect(FractalParams.WIDTH * FractalParams.HEIGHT * 4);
        int[] pixels = image.getRGB(0, 0, FractalParams.WIDTH, FractalParams.HEIGHT, null, 0, FractalParams.WIDTH);
        for (int argb : pixels) {
            buffer.put((byte) ((argb >> 16) & 0xFF)); // R
            buffer.put((byte) ((argb >> 8)  & 0xFF)); // G
            buffer.put((byte) ( argb        & 0xFF)); // B
            buffer.put((byte) ((argb >> 24) & 0xFF)); // A
        }
        buffer.flip();

        glBindTexture(GL_TEXTURE_2D, overlayTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FractalParams.WIDTH, FractalParams.HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, overlayTextureID);
        glBegin(GL_QUADS);
        {
            glTexCoord2d(0.0f, 1f); glVertex2d(-1, -1);
            glTexCoord2d(0.0f, 0.0f); glVertex2d(-1,  1);
            glTexCoord2d(1f,   0.0f); glVertex2d( 1,  1);
            glTexCoord2d(1f,   1f);   glVertex2d( 1, -1);
        }
        glEnd();
        glDisable(GL_BLEND);
    }

    public static void main(String[] args) {
        new FractalMain().run();
    }

}
