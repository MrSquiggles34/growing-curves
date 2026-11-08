#include "collisionDetector.h"
#include "segmentGeometry.h"

#include <glm/common.hpp>


CollisionDetector::CollisionDetector(
    const GeometryNetwork& network,
    const SpatialGrid& spatialGrid)
    : network(network),
    spatialGrid(spatialGrid)
{
}

bool CollisionDetector::wouldCollide(
    const glm::vec2& start,
    const glm::vec2& end,
    float radius,
    int curveId) const
{
    std::vector<EdgeId> candidates =
        spatialGrid.getCandidateEdges(start, end, radius);

    totalCollisionQueries++;
    totalCandidateEdges += candidates.size();

    for (EdgeId edgeId : candidates)
    {
        const Edge& edge = network.getEdge(edgeId);

        // Ignore edges belonging to the current curve.
        if (edge.curveId == curveId)
        {
            continue;
        }

        const glm::vec2& edgeStart =
            network.getVertex(edge.startVertex).position;

        const glm::vec2& edgeEnd =
            network.getVertex(edge.endVertex).position;

        totalIntersectionTests++;

        if (capsulesIntersect(
            start,
            end,
            radius,
            edgeStart,
            edgeEnd,
            edge.radius))
        {
            return true;
        }
    }

    return false;
}

glm::vec2 CollisionDetector::findLastSafePosition(
    const glm::vec2& start,
    const glm::vec2& end,
    float radius,
    int curveId
) const
{
    // If the entire movement is safe, accept it.
    if (!wouldCollide(start, end, radius, curveId))
    {
        return end;
    }

    float low = 0.0f;
    float high = 1.0f;

    // Find the earliest collision along the proposed movement.
    for (int i = 0; i < 20; ++i)
    {
        float middle = (low + high) * 0.5f;
        glm::vec2 candidate = glm::mix(start, end, middle);

        if (wouldCollide(start, candidate, radius, curveId))
        {
            high = middle;
        }
        else
        {
            low = middle;
        }
    }

    // Back off slightly from the contact position.
    float safeT = low;

    glm::vec2 movement = end - start;
    float movementLength = glm::length(movement);

    if (movementLength > 0.0f)
    {
        float safetyDistance = 0.001f;
        safeT -= safetyDistance / movementLength;
    }

    safeT = glm::clamp(safeT, 0.0f, 1.0f);

    return glm::mix(start, end, safeT);
}

std::size_t CollisionDetector::getTotalCandidateEdges() const
{
    return totalCandidateEdges;
}

std::size_t CollisionDetector::getTotalIntersectionTests() const
{
    return totalIntersectionTests;
}

std::size_t CollisionDetector::getTotalCollisionQueries() const
{
    return totalCollisionQueries;
}