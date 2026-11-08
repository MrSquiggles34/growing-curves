#include "geometryNetwork.h"

#include <stdexcept>

VertexId GeometryNetwork::addVertex(const glm::vec2& position)
{
    VertexId id = vertices.size();

    Vertex vertex;
    vertex.id = id;
    vertex.position = position;

    vertices.push_back(vertex);

    return id;
}

EdgeId GeometryNetwork::addEdge(
    VertexId start,
    VertexId end,
    int curveId,
    float radius
)
{
    // Make sure both endpoint vertices exist.
    if (start >= vertices.size() || end >= vertices.size())
    {
        throw std::out_of_range("GeometryNetwork: Invalid vertex ID");
    }

    if (start == end)
    {
        throw std::invalid_argument("GeometryNetwork: Edge cannot connect a vertex to itself");
    }

    if (radius <= 0.0f)
    {
        throw std::invalid_argument("GeometryNetwork: Edge radius must be positive");
    }

    EdgeId id = edges.size();

    Edge edge;
    edge.id = id;
    edge.startVertex = start;
    edge.endVertex = end;
    edge.radius = radius;
    edge.curveId = curveId;

    edges.push_back(edge);

    // Register this edge with both endpoint vertices.
    vertices[start].connectedEdges.push_back(id);
    vertices[end].connectedEdges.push_back(id);

    return id;
}

void GeometryNetwork::addPolyline(
    const std::vector<glm::vec2>& points,
    int curveId,
    float radius
)
{
    if (points.size() < 2)
    {
        return;
    }

    // Create a vertex for the first point.
    VertexId previousVertex = addVertex(points[0]);

    // Create the remaining vertices and connect them.
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        VertexId currentVertex = addVertex(points[i]);

        addEdge(previousVertex, currentVertex, curveId, radius);

        previousVertex = currentVertex;
    }
}

const Vertex& GeometryNetwork::getVertex(VertexId id) const
{
    return vertices.at(id);
}

const Edge& GeometryNetwork::getEdge(EdgeId id) const
{
    return edges.at(id);
}

const std::vector<Vertex>& GeometryNetwork::getVertices() const
{
    return vertices;
}

const std::vector<Edge>& GeometryNetwork::getEdges() const
{
    return edges;
}

std::size_t GeometryNetwork::getVertexCount() const
{
    return vertices.size();
}

std::size_t GeometryNetwork::getEdgeCount() const
{
    return edges.size();
}