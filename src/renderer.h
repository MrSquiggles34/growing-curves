
#pragma once

#include "geometry/geometryNetwork.h"

#include <glad/glad.h>
#include <glm/vec3.hpp>

#include <vector>

class Renderer
{
public:
    Renderer(GLuint shaderProgram, int width, int height);
    ~Renderer();

    void render(
        const GeometryNetwork& network,
        const glm::vec3& color
    );

private:
    GLuint shaderProgram;
    GLuint vao;
    GLuint vbo;

    int width;
    int height;

    std::vector<float> vertices;

    void addEdgeGeometry(
        const glm::vec2& start,
        const glm::vec2& end,
        float radius
    );

    void addVertex(
        const glm::vec2& position
    );
};
