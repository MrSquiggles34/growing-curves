#pragma once

#include "geometryNetwork.h"
#include "spatialGrid.h"

#include <glm/vec2.hpp>

struct CollisionResult
{
    bool collided = false;

    bool hasEdge = false;

    EdgeId edgeId = 0;

    // Last safe position of the growing curve.
    glm::vec2 safePosition;

    // Position on the existing edge's centerline.
    glm::vec2 edgePosition;

    // Position along the existing edge, from 0.0 to 1.0.
    float edgeParameter = 0.0f;
};

class CollisionDetector
{
public:
    CollisionDetector(
        const GeometryNetwork& network,
        const SpatialGrid& spatialGrid
    );

    // Returns true if the proposed capsule collides with another curve.
    bool wouldCollide(
        const glm::vec2& start,
        const glm::vec2& end,
        float radius,
        int curveId
    ) const;

    glm::vec2 findLastSafePosition(
        const glm::vec2& start,
        const glm::vec2& end,
        float radius,
        int curveId
    ) const;

    CollisionResult findCollision(
        const glm::vec2& start,
        const glm::vec2& end,
        float radius,
        int curveId
    ) const;

    std::size_t getTotalCandidateEdges() const;
    std::size_t getTotalIntersectionTests() const;
    std::size_t getTotalCollisionQueries() const;

private:
    const GeometryNetwork& network;
    const SpatialGrid& spatialGrid;

    mutable std::size_t totalCandidateEdges = 0;
    mutable std::size_t totalIntersectionTests = 0;
    mutable std::size_t totalCollisionQueries = 0;
};