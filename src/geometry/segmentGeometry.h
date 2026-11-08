#pragma once

#include <glm/glm.hpp>


// Returns the minimum squared distance between two line segments.
float segmentSegmentDistanceSquared(
    const glm::vec2& a0,
    const glm::vec2& a1,
    const glm::vec2& b0,
    const glm::vec2& b1
);

// Returns true if two capsules touch or overlap.
bool capsulesIntersect(
    const glm::vec2& a0,
    const glm::vec2& a1,
    float radiusA,
    const glm::vec2& b0,
    const glm::vec2& b1,
    float radiusB
);