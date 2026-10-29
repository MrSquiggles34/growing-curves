#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

#include "directionField.h"
#include "particle.h"
#include "curveGrower.h"
#include "straightDirectionField.h"
#include "distanceMap.h"
#include "simulation.h"
#include "occupancyGrid.h"

static std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Cannot open shader: " + path);
    }

    std::stringstream buffer;

    buffer << file.rdbuf();
    return buffer.str();
}

static GLuint compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* text = source.c_str();

    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compilation failed:\n" << log << '\n';
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void setPixel(
    std::vector<unsigned char>& pixels,
    int width,
    int height,
    int x,
    int y,
    unsigned char r,
    unsigned char g,
    unsigned char b)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return;
    }

    std::size_t index = (y * width + x) * 4;

    pixels[index + 0] = r;
    pixels[index + 1] = g;
    pixels[index + 2] = b;
    pixels[index + 3] = 255;
}

void drawLine(
    std::vector<unsigned char>& pixels,
    int width,
    int height,
    glm::vec2 start,
    glm::vec2 end,
    unsigned char r,
    unsigned char g,
    unsigned char b)
{
    int x0 = static_cast<int>(std::round(start.x));
    int y0 = static_cast<int>(std::round(start.y));
    int x1 = static_cast<int>(std::round(end.x));
    int y1 = static_cast<int>(std::round(end.y));

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);

    int stepX = (x0 < x1) ? 1 : -1;
    int stepY = (y0 < y1) ? 1 : -1;

    int error = dx - dy;

    while (true)
    {
        setPixel(
            pixels, width, height,
            x0, y0, r, g, b);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int doubledError = 2 * error;

        if (doubledError > -dy)
        {
            error -= dy;
            x0 += stepX;
        }

        if (doubledError < dx)
        {
            error += dx;
            y0 += stepY;
        }
    }
}

void drawCurve(
    std::vector<unsigned char>& pixels,
    int width,
    int height,
    const Curve& curve,
    unsigned char r,
    unsigned char g,
    unsigned char b)
{
    for (std::size_t i = 1; i < curve.points.size(); ++i)
    {
        drawLine(
            pixels,
            width,
            height,
            curve.points[i - 1],
            curve.points[i],
            r, g, b);
    }
}

static GLuint createProgram()
{
    const std::string root = SIM_SOURCE_DIR;

	// Compile shaders.
    GLuint vertex = compileShader(GL_VERTEX_SHADER, readFile(root + "/shaders/texture.vert"));
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, readFile(root + "/shaders/texture.frag"));

	// Delete shaders if compilation failed.
    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return 0;
    }

	// Create program and link shaders.
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Shader linking failed:\n" << log << '\n';
        glDeleteProgram(program);
        return 0;
    }

    return program;
}



int main()
{
	// Create a window and OpenGL context.
    constexpr int imageWidth = 256;
    constexpr int imageHeight = 256;

    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(256, 256, "2D Simulation Texture", nullptr, nullptr);

    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "GLAD initialization failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint program = 0;

    try {
        program = createProgram();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Two triangles forming a full-screen rectangle quad.
    const float vertices[] = {
        // position       // texture coordinates
        -1.0f, -1.0f,     0.0f, 0.0f,
         1.0f, -1.0f,     1.0f, 0.0f,
         1.0f,  1.0f,     1.0f, 1.0f,
        -1.0f,  1.0f,     0.0f, 1.0f
    };

    const unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

	// Create GPU resources
    GLuint vao, vbo, ebo; 
	glGenVertexArrays(1, &vao);         // vertex array object stores vertex attribute configuration
	glGenBuffers(1, &vbo);              // vertex buffer object stores vertex data
	glGenBuffers(1, &ebo);              // element buffer object stores indicies specifying how to draw the vertices

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Configure vertex attributes: position (location 0) and texture coordinates (location 1)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // CPU-side image: RGBA, 8 bits per channel.
    std::vector<unsigned char> pixels(imageWidth * imageHeight * 4, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Start Simulation
    Simulation simulation(imageWidth, imageHeight);

    // Initialize the image to opaque black.
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i + 0] = 0;
        pixels[i + 1] = 0;
        pixels[i + 2] = 0;
        pixels[i + 3] = 255;
    }

    // Advance the simulation and measure its execution time.
    int stepCount = 0;

    double startTime = glfwGetTime();

    while (stepCount < 100000000 && !simulation.isFinished())
    {
        simulation.step(0.01f);
        ++stepCount;
    }

    double endTime = glfwGetTime();

    double elapsedMilliseconds = (endTime - startTime) * 1000.0;

    std::cout << "Simulation time: "
        << elapsedMilliseconds << " ms" << std::endl;

    std::cout << "Simulation steps: "
        << stepCount << std::endl;

    std::cout << "Simulation finished: "
        << (simulation.isFinished() ? "yes" : "no")
        << std::endl;

    // Draw completed curves in red.
    for (const Curve& curve : simulation.getCurves())
    {
        drawCurve(
            pixels,
            imageWidth,
            imageHeight,
            curve,
            255, 60, 60
        );
    }

    // Draw the currently growing curve in white, if one exists.
    const Curve* currentCurve = simulation.getCurrentCurve();

    if (currentCurve != nullptr)
    {
        drawCurve(
            pixels,
            imageWidth,
            imageHeight,
            *currentCurve,
            255, 255, 255
        );
    }

    std::cout << "Completed curves: "
        << simulation.getCurves().size() << '\n';

    std::cout << "Maximum remaining distance: "
        << simulation.getMaximumRemainingDistance() << '\n';

    glm::vec2 start = simulation.getCurrentStartPosition();

    std::cout << "Last starting point: ("
        << start.x << ", " << start.y << ")\n";

    std::cout << "Simulation finished: "
        << (simulation.isFinished() ? "YES" : "NO") << '\n';

    std::cout << "Stop reason: ";

    switch (simulation.getStopReason())
    {
    case Simulation::StopReason::MinimumDistanceReached:
        std::cout << "Minimum distance reached\n";
        break;

    case Simulation::StopReason::MaximumCurvesReached:
        std::cout << "Maximum curves reached\n";
        break;

    case Simulation::StopReason::NoValidStartingPoint:
        std::cout << "No valid starting point\n";
        break;

    default:
        std::cout << "Still running or not set\n";
        break;
    }


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

		// Update the CPU-side image




        // Upload this frame's image to the GPU texture.
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.04f, 0.04f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
    }

	// Clean up resources.
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}