#include "segmentGeometry.h"

#include <algorithm>

namespace
{
    constexpr float EPSILON = 0.000001f;

    float cross2D(const glm::vec2& a, const glm::vec2& b)
    {
        return a.x * b.y - a.y * b.x;
    }

    bool pointOnSegment(
        const glm::vec2& point,
        const glm::vec2& start,
        const glm::vec2& end
    )
    {
        glm::vec2 segment = end - start;
        glm::vec2 toPoint = point - start;

        float segmentLengthSquared = glm::dot(segment, segment);

        if (segmentLengthSquared <= EPSILON * EPSILON)
        {
            glm::vec2 difference = point - start;
            return glm::dot(difference, difference) <= EPSILON * EPSILON;
        }

        if (std::abs(cross2D(segment, toPoint)) > EPSILON)
        {
            return false;
        }

        float projection = glm::dot(toPoint, segment);

        return projection >= -EPSILON &&
            projection <= segmentLengthSquared + EPSILON;
    }

    bool segmentsIntersect(
        const glm::vec2& a0,
        const glm::vec2& a1,
        const glm::vec2& b0,
        const glm::vec2& b1
    )
    {
        glm::vec2 r = a1 - a0;
        glm::vec2 s = b1 - b0;

        float denominator = cross2D(r, s);
        float numeratorT = cross2D(b0 - a0, s);
        float numeratorU = cross2D(b0 - a0, r);

        // Parallel or collinear segments.
        if (std::abs(denominator) <= EPSILON)
        {
            if (std::abs(cross2D(b0 - a0, r)) > EPSILON)
            {
                return false;
            }

            return pointOnSegment(a0, b0, b1) ||
                pointOnSegment(a1, b0, b1) ||
                pointOnSegment(b0, a0, a1) ||
                pointOnSegment(b1, a0, a1);
        }

        float t = numeratorT / denominator;
        float u = numeratorU / denominator;

        return t >= -EPSILON && t <= 1.0f + EPSILON &&
            u >= -EPSILON && u <= 1.0f + EPSILON;
    }

    float pointSegmentDistanceSquared(
        const glm::vec2& point,
        const glm::vec2& start,
        const glm::vec2& end
    )
    {
        glm::vec2 segment = end - start;
        float lengthSquared = glm::dot(segment, segment);

        // Handle a zero-length segment.
        if (lengthSquared <= EPSILON * EPSILON)
        {
            glm::vec2 difference = point - start;
            return glm::dot(difference, difference);
        }

        float t = glm::dot(point - start, segment) / lengthSquared;
        t = std::clamp(t, 0.0f, 1.0f);

        glm::vec2 closestPoint = start + t * segment;
        glm::vec2 difference = point - closestPoint;

        return glm::dot(difference, difference);
    }
}

float segmentSegmentDistanceSquared(
    const glm::vec2& a0,
    const glm::vec2& a1,
    const glm::vec2& b0,
    const glm::vec2& b1
)
{
    // Intersecting segments have zero distance.
    if (segmentsIntersect(a0, a1, b0, b1))
    {
        return 0.0f;
    }

    // Otherwise, the closest points involve at least one endpoint.
    float distanceA0 = pointSegmentDistanceSquared(a0, b0, b1);
    float distanceA1 = pointSegmentDistanceSquared(a1, b0, b1);
    float distanceB0 = pointSegmentDistanceSquared(b0, a0, a1);
    float distanceB1 = pointSegmentDistanceSquared(b1, a0, a1);

    return std::min(
        std::min(distanceA0, distanceA1),
        std::min(distanceB0, distanceB1)
    );
}

bool capsulesIntersect(
    const glm::vec2& a0,
    const glm::vec2& a1,
    float radiusA,
    const glm::vec2& b0,
    const glm::vec2& b1,
    float radiusB
)
{
    float combinedRadius = radiusA + radiusB;
    float combinedRadiusSquared = combinedRadius * combinedRadius;

    float distanceSquared = segmentSegmentDistanceSquared(
        a0, a1,
        b0, b1
    );

    return distanceSquared <= combinedRadiusSquared;
}