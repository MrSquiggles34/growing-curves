
#include "CurveGrower.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <stdexcept>

namespace
{
    float cross2D(glm::vec2 a, glm::vec2 b)
    {
        return a.x * b.y - a.y * b.x;
    }

    glm::vec2 validatedDirection(glm::vec2 direction)
    {
        float magnitude = glm::length(direction);

        if (magnitude == 0.0f)
        {
            throw std::invalid_argument(
                "Initial direction must not be zero.");
        }

        return direction / magnitude;
    }
}

CurveGrower::CurveGrower(
    glm::vec2 startPosition,
    glm::vec2 initialDirection,
    float initialSpeed,
    glm::vec2 regionMin,
    glm::vec2 regionMax,
    CollisionDetector* collisionDetector,
    int curveId)
    : forwardParticle(
        startPosition,
        validatedDirection(initialDirection)* initialSpeed),
    backwardParticle(
        startPosition,
        -validatedDirection(initialDirection) * initialSpeed),
    regionMin(regionMin),
    regionMax(regionMax),
    collisionDetector(collisionDetector),
    curveId(curveId)
{
    if (regionMin.x > regionMax.x ||
        regionMin.y > regionMax.y)
    {
        throw std::invalid_argument(
            "Invalid growth region bounds.");
    }

    if (!isInsideRegion(startPosition))
    {
        throw std::invalid_argument(
            "Starting position must be inside the growth region.");
    }

    forwardPoints.push_back(startPosition);
    backwardPoints.push_back(startPosition);

    rebuildCurve();
}

bool CurveGrower::isInsideRegion(glm::vec2 position) const
{
    return position.x >= regionMin.x &&
        position.x <= regionMax.x &&
        position.y >= regionMin.y &&
        position.y <= regionMax.y;
}

glm::vec2 CurveGrower::clipToBoundary(
    glm::vec2 start,
    glm::vec2 end) const
{
    glm::vec2 delta = end - start;

    float t = 1.0f;

    // Find the first crossing of a vertical boundary.
    if (delta.x > 0.0f && end.x > regionMax.x)
    {
        t = std::min(
            t, (regionMax.x - start.x) / delta.x);
    }
    else if (delta.x < 0.0f && end.x < regionMin.x)
    {
        t = std::min(
            t, (regionMin.x - start.x) / delta.x);
    }

    // Find the first crossing of a horizontal boundary.
    if (delta.y > 0.0f && end.y > regionMax.y)
    {
        t = std::min(
            t, (regionMax.y - start.y) / delta.y);
    }
    else if (delta.y < 0.0f && end.y < regionMin.y)
    {
        t = std::min(
            t, (regionMin.y - start.y) / delta.y);
    }

    return start + t * delta;
}

void CurveGrower::rebuildCurve()
{
    curve.points.clear();
    curve.length = 0.0f;

    // Reverse the backward branch: tip to starting point.
    for (auto it = backwardPoints.rbegin();
        it != backwardPoints.rend(); ++it)
    {
        curve.points.push_back(*it);
    }

    // Append the forward branch, skipping the duplicated start.
    for (std::size_t i = 1; i < forwardPoints.size(); ++i)
    {
        curve.points.push_back(forwardPoints[i]);
    }

    // Calculate the total polyline length.
    for (std::size_t i = 1; i < curve.points.size(); ++i)
    {
        curve.length += glm::distance(
            curve.points[i - 1],
            curve.points[i]);
    }
}

const Curve& CurveGrower::getCurve() const
{
    return curve;
}

int CurveGrower::getCurveId() const
{
    return curveId;
}

bool CurveGrower::isFinished() const
{
    return !forwardActive && !backwardActive;
}

void CurveGrower::finalizeCurve()
{
    rebuildCurve();
}