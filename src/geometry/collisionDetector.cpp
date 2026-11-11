#include "collisionDetector.h"
#include "segmentGeometry.h"

#include <glm/common.hpp>
#include <algorithm>
#include <limits>
#include <glm/geometric.hpp>

namespace
{
    float closestPointOnSegment(
        const glm::vec2& point,
        const glm::vec2& start,
        const glm::vec2& end,
        glm::vec2& closestPoint)
    {
        glm::vec2 segment = end - start;

        float segmentLengthSquared = glm::dot(segment, segment);

        if (segmentLengthSquared <= 0.000001f)
        {
            closestPoint = start;
            return 0.0f;
        }

        float t = glm::dot(point - start, segment)
            / segmentLengthSquared;

        t = std::clamp(t, 0.0f, 1.0f);

        closestPoint = start + t * segment;

        return t;
    }
}

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

CollisionResult CollisionDetector::findCollision(
    const glm::vec2& start,
    const glm::vec2& end,
    float radius,
    int curveId) const
{
    CollisionResult result;

    // First, check whether this movement actually collides.
    if (!wouldCollide(start, end, radius, curveId))
    {
        return result;
    }

    result.collided = true;

    // Find the last safe position before the collision.
    result.safePosition = findLastSafePosition(
        start,
        end,
        radius,
        curveId
    );

    // Retrieve nearby edges using the spatial grid.
    std::vector<EdgeId> candidates =
        spatialGrid.getCandidateEdges(start, end, radius);

    float closestDistanceSquared =
        std::numeric_limits<float>::max();

    EdgeId closestEdgeId = 0;
    glm::vec2 closestEdgePosition(0.0f);
    float closestEdgeParameter = 0.0f;

    // Find the existing edge closest to the safe position.
    for (EdgeId edgeId : candidates)
    {
        const Edge& edge = network.getEdge(edgeId);

        if (edge.curveId == curveId)
        {
            continue;
        }

        const glm::vec2& edgeStart =
            network.getVertex(edge.startVertex).position;

        const glm::vec2& edgeEnd =
            network.getVertex(edge.endVertex).position;

        glm::vec2 pointOnEdge;

        float edgeParameter = closestPointOnSegment(
            result.safePosition,
            edgeStart,
            edgeEnd,
            pointOnEdge
        );

        glm::vec2 difference =
            result.safePosition - pointOnEdge;

        float distanceSquared = glm::dot(
            difference,
            difference
        );

        if (distanceSquared < closestDistanceSquared)
        {
            closestDistanceSquared = distanceSquared;

            closestEdgeId = edgeId;
            closestEdgePosition = pointOnEdge;
            closestEdgeParameter = edgeParameter;
        }
    }

    if (closestDistanceSquared == std::numeric_limits<float>::max())
    {
        result.collided = false;
        return result;
    }

    result.hasEdge = true;

    result.edgeId = closestEdgeId;
    result.edgePosition = closestEdgePosition;
    result.edgeParameter = closestEdgeParameter;

    return result;
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