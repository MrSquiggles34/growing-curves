#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

static GLuint createProgram()
{
    const std::string root = SIM_SOURCE_DIR;

    GLuint vertex = compileShader(
        GL_VERTEX_SHADER,
        readFile(root + "/shaders/texture.vert"));

    GLuint fragment = compileShader(
        GL_FRAGMENT_SHADER,
        readFile(root + "/shaders/texture.frag"));

    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return 0;
    }

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
    constexpr int imageWidth = 256;
    constexpr int imageHeight = 256;

    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        900, 700, "2D Simulation Texture", nullptr, nullptr);

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

    // Two triangles forming a full-screen rectangle.
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

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
        vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
        indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // CPU-side image: RGBA, 8 bits per channel.
    std::vector<unsigned char> pixels(
        imageWidth * imageHeight * 4, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        imageWidth, imageHeight, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    double startTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Example simulation output: a moving Gaussian spot.
        const float t = static_cast<float>(glfwGetTime() - startTime);
        const float cx = 128.0f + 65.0f * std::sin(t * 0.8f);
        const float cy = 128.0f + 65.0f * std::cos(t * 0.6f);

        for (int y = 0; y < imageHeight; ++y) {
            for (int x = 0; x < imageWidth; ++x) {
                const float dx = x - cx;
                const float dy = y - cy;
                const float d2 = dx * dx + dy * dy;

                const float intensity =
                    std::exp(-d2 / (2.0f * 18.0f * 18.0f));

                const int i = (y * imageWidth + x) * 4;

                pixels[i + 0] = static_cast<unsigned char>(
                    30.0f + 40.0f * intensity);
                pixels[i + 1] = static_cast<unsigned char>(
                    35.0f + 210.0f * intensity);
                pixels[i + 2] = static_cast<unsigned char>(
                    45.0f + 180.0f * intensity);
                pixels[i + 3] = 255;
            }
        }

        // Upload this frame's image to the GPU texture.
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0,
            imageWidth, imageHeight,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

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

    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}