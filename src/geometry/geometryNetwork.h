#pragma once

#include <cstddef>
#include <vector>
#include <string>

#include <glm/vec2.hpp>

using VertexId = std::size_t;
using EdgeId = std::size_t;

struct Vertex
{
    VertexId id;
    glm::vec2 position;

    // Edges that begin or end at this vertex.
    std::vector<EdgeId> connectedEdges;
};

struct Edge
{
    EdgeId id;

    VertexId startVertex;
    VertexId endVertex;

    // Radius of the 2D capsule surrounding this edge.
    float radius;

    // Identifies which curve created this edge.
    int curveId;
};

struct GraphComponentStatistics
{
    std::size_t vertexCount = 0;
    std::size_t edgeCount = 0;
};

struct GraphStatistics
{
    std::size_t connectedComponents = 0;

    std::size_t isolatedVertices = 0;
    std::size_t degree1Vertices = 0;
    std::size_t degree2Vertices = 0;
    std::size_t degree3OrMoreVertices = 0;

    std::size_t maximumDegree = 0;
    std::size_t independentCycles = 0;

    std::vector<GraphComponentStatistics> components;
};

class GeometryNetwork
{
public:
    VertexId addVertex(const glm::vec2& position);

    EdgeId addEdge(
        VertexId start,
        VertexId end,
        int curveId,
        float radius = 1.0f
    );

    VertexId splitEdge(
        EdgeId edgeId,
        const glm::vec2& position
    );

    void addPolyline(
        const std::vector<glm::vec2>& points,
        int curveId,
        float radius = 1.0f
    );

    void addPolylineWithEndpoints(
        const std::vector<glm::vec2>& points,
        int curveId,
        bool useStartVertex,
        VertexId startVertex,
        bool useEndVertex,
        VertexId endVertex,
        float radius = 1.0f
    );

    // Validate
    bool validate(std::string& error) const;

    GraphStatistics calculateStatistics() const;

    // Getters
    const Vertex& getVertex(VertexId id) const;
    const Edge& getEdge(EdgeId id) const;

    const std::vector<Vertex>& getVertices() const;
    const std::vector<Edge>& getEdges() const;

    std::size_t getVertexCount() const;
    std::size_t getEdgeCount() const;




private:
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
};