
#include "Renderer.h"

#include <glm/geometric.hpp>
#include <algorithm>
#include <iostream>

Renderer::Renderer(GLuint shaderProgram, int width, int height)
    : shaderProgram(shaderProgram),
    vao(0),
    vbo(0),
    width(width),
    height(height)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Renderer::addVertex(const glm::vec2& position)
{
    // Convert simulation coordinates to OpenGL NDC.
    float x = (position.x / width) * 2.0f - 1.0f;
    float y = 1.0f - (position.y / height) * 2.0f;

    vertices.push_back(x);
    vertices.push_back(y);
}

void Renderer::addEdgeGeometry(
    const glm::vec2& start,
    const glm::vec2& end,
    float radius)
{
    glm::vec2 direction = end - start;

    float length = glm::length(direction);

    if (length < 0.0001f)
        return;

    direction /= length;

    // Perpendicular direction determines the edge width.
    glm::vec2 perpendicular(
        -direction.y,
        direction.x
    );

    glm::vec2 offset = perpendicular * radius;

    glm::vec2 a = start + offset;
    glm::vec2 b = start - offset;
    glm::vec2 c = end + offset;
    glm::vec2 d = end - offset;

    // First triangle.
    addVertex(a);
    addVertex(b);
    addVertex(c);

    // Second triangle.
    addVertex(c);
    addVertex(b);
    addVertex(d);
}

void Renderer::render(
    const GeometryNetwork& network,
    const glm::vec3& color)
{
    vertices.clear();

    const std::vector<Edge>& edges = network.getEdges();

    // Generate geometry for every graph edge.
    for (const Edge& edge : edges)
    {
        const Vertex& start =
            network.getVertex(edge.startVertex);

        const Vertex& end =
            network.getVertex(edge.endVertex);

        addEdgeGeometry(
            start.position,
            end.position,
            std::max(edge.radius, 0.5f)
        );
    }

    if (vertices.empty())
        return;

    glUseProgram(shaderProgram);

    GLint colorLocation =
        glGetUniformLocation(shaderProgram, "uColor");

    glUniform3fv(colorLocation, 1, &color[0]);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(vertices.size() / 2)
    );

    glBindVertexArray(0);
}
