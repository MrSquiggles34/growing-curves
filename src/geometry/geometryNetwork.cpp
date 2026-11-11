#include "geometryNetwork.h"

#include <stdexcept>
#include <algorithm>
#include <glm/geometric.hpp>
#include <queue>

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

VertexId GeometryNetwork::splitEdge(
    EdgeId edgeId,
    const glm::vec2& position)
{
    if (edgeId >= edges.size())
    {
        throw std::out_of_range("Invalid edge ID");
    }

    // Copy the original edge information before modifying anything.
    Edge originalEdge = edges[edgeId];

    VertexId oldStart = originalEdge.startVertex;
    VertexId oldEnd = originalEdge.endVertex;

    const glm::vec2& startPosition =
        vertices[oldStart].position;

    const glm::vec2& endPosition =
        vertices[oldEnd].position;

    // Don't allow a split directly on an existing endpoint.
    const float epsilon = 0.0001f;

    if (glm::distance(position, startPosition) <= epsilon ||
        glm::distance(position, endPosition) <= epsilon)
    {
        throw std::invalid_argument(
            "Cannot split an edge at an existing endpoint"
        );
    }

    // Create the new vertex.
    VertexId newVertex = addVertex(position);

    // Remove the original edge from the old end vertex.
    std::vector<EdgeId>& oldEndEdges =
        vertices[oldEnd].connectedEdges;

    oldEndEdges.erase(
        std::remove(
            oldEndEdges.begin(),
            oldEndEdges.end(),
            edgeId
        ),
        oldEndEdges.end()
    );

    // Change the original edge from A-B to A-C.
    edges[edgeId].endVertex = newVertex;

    // Connect the new vertex to the original edge.
    vertices[newVertex].connectedEdges.push_back(edgeId);

    // Create the second edge, C-B.
    EdgeId newEdge = addEdge(
        newVertex,
        oldEnd,
        originalEdge.curveId,
        originalEdge.radius
    );

    return newVertex;
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

void GeometryNetwork::addPolylineWithEndpoints(
    const std::vector<glm::vec2>& points,
    int curveId,
    bool useStartVertex,
    VertexId startVertex,
    bool useEndVertex,
    VertexId endVertex,
    float radius)
{
    if (points.size() < 2)
    {
        return;
    }

    if (useStartVertex && startVertex >= vertices.size())
    {
        throw std::out_of_range("Invalid start vertex ID");
    }

    if (useEndVertex && endVertex >= vertices.size())
    {
        throw std::out_of_range("Invalid end vertex ID");
    }

    VertexId previousVertex;

    // Reuse the existing start vertex, or create a new one.
    if (useStartVertex)
    {
        previousVertex = startVertex;
    }
    else
    {
        previousVertex = addVertex(points.front());
    }

    // Create the remaining vertices and edges.
    for (std::size_t i = 1; i < points.size(); i++)
    {
        VertexId currentVertex;

        // Reuse the existing end vertex on the final point.
        if (i == points.size() - 1 && useEndVertex)
        {
            currentVertex = endVertex;
        }
        else
        {
            currentVertex = addVertex(points[i]);
        }

        addEdge(
            previousVertex,
            currentVertex,
            curveId,
            radius
        );

        previousVertex = currentVertex;
    }
}

bool GeometryNetwork::validate(std::string& error) const
{
    // Check every vertex.
    for (std::size_t i = 0; i < vertices.size(); i++)
    {
        const Vertex& vertex = vertices[i];

        // The vertex ID should match its position in the array.
        if (vertex.id != i)
        {
            error = "Vertex ID does not match its array index.";
            return false;
        }

        // Check every edge connected to this vertex.
        for (std::size_t j = 0; j < vertex.connectedEdges.size(); j++)
        {
            EdgeId edgeId = vertex.connectedEdges[j];

            if (edgeId >= edges.size())
            {
                error = "Vertex references an invalid edge ID.";
                return false;
            }

            const Edge& edge = edges[edgeId];

            // The edge must actually use this vertex.
            if (edge.startVertex != vertex.id &&
                edge.endVertex != vertex.id)
            {
                error = "Vertex references an edge that does not use it.";
                return false;
            }

            // An edge should only appear once in a vertex's connection list.
            for (std::size_t k = j + 1;
                k < vertex.connectedEdges.size();
                k++)
            {
                if (vertex.connectedEdges[k] == edgeId)
                {
                    error = "Duplicate edge reference in vertex.";
                    return false;
                }
            }
        }
    }

    // Check every edge.
    for (std::size_t i = 0; i < edges.size(); i++)
    {
        const Edge& edge = edges[i];

        if (edge.id != i)
        {
            error = "Edge ID does not match its array index.";
            return false;
        }

        if (edge.startVertex >= vertices.size() ||
            edge.endVertex >= vertices.size())
        {
            error = "Edge references an invalid vertex ID.";
            return false;
        }

        if (edge.startVertex == edge.endVertex)
        {
            error = "Edge connects a vertex to itself.";
            return false;
        }

        const Vertex& start = vertices[edge.startVertex];
        const Vertex& end = vertices[edge.endVertex];

        // The start vertex must reference this edge.
        int startCount = 0;

        for (EdgeId connectedEdge : start.connectedEdges)
        {
            if (connectedEdge == edge.id)
                startCount++;
        }

        if (startCount != 1)
        {
            error = "Edge is not registered exactly once with its start vertex.";
            return false;
        }

        // The end vertex must reference this edge.
        int endCount = 0;

        for (EdgeId connectedEdge : end.connectedEdges)
        {
            if (connectedEdge == edge.id)
                endCount++;
        }

        if (endCount != 1)
        {
            error = "Edge is not registered exactly once with its end vertex.";
            return false;
        }
    }

    error.clear();
    return true;
}

GraphStatistics GeometryNetwork::calculateStatistics() const
{
    GraphStatistics statistics;

    // ------------------------------------------------
    // 1. Calculate vertex degree distribution.
    // ------------------------------------------------

    for (const Vertex& vertex : vertices)
    {
        std::size_t degree = vertex.connectedEdges.size();

        if (degree == 0)
        {
            statistics.isolatedVertices++;
        }
        else if (degree == 1)
        {
            statistics.degree1Vertices++;
        }
        else if (degree == 2)
        {
            statistics.degree2Vertices++;
        }
        else
        {
            statistics.degree3OrMoreVertices++;
        }

        statistics.maximumDegree = std::max(
            statistics.maximumDegree,
            degree
        );
    }

    // ------------------------------------------------
// 2. Find connected components and their sizes.
// ------------------------------------------------

// Store the component index assigned to each vertex.
    std::vector<std::size_t> componentIds(
        vertices.size(),
        vertices.size()
    );

    std::vector<bool> visited(vertices.size(), false);

    for (std::size_t i = 0; i < vertices.size(); i++)
    {
        if (visited[i])
        {
            continue;
        }

        std::size_t componentIndex =
            statistics.connectedComponents;

        statistics.connectedComponents++;

        GraphComponentStatistics component;

        std::queue<VertexId> queue;

        queue.push(i);
        visited[i] = true;
        componentIds[i] = componentIndex;

        while (!queue.empty())
        {
            VertexId currentVertexId = queue.front();
            queue.pop();

            component.vertexCount++;

            const Vertex& currentVertex =
                vertices[currentVertexId];

            for (EdgeId edgeId : currentVertex.connectedEdges)
            {
                const Edge& edge = edges[edgeId];

                VertexId neighbor;

                if (edge.startVertex == currentVertexId)
                {
                    neighbor = edge.endVertex;
                }
                else
                {
                    neighbor = edge.startVertex;
                }

                if (!visited[neighbor])
                {
                    visited[neighbor] = true;
                    componentIds[neighbor] = componentIndex;

                    queue.push(neighbor);
                }
            }
        }

        statistics.components.push_back(component);
    }

    // Count edges belonging to each component.
    // Every edge belongs to the component of its start vertex.
    for (const Edge& edge : edges)
    {
        std::size_t componentIndex =
            componentIds[edge.startVertex];

        statistics.components[componentIndex].edgeCount++;
    }

    // ------------------------------------------------
    // 3. Calculate independent cycle count.
    // ------------------------------------------------

    statistics.independentCycles =
        edges.size()
        + statistics.connectedComponents
        - vertices.size();

    return statistics;
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