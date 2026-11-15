
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
    glm::vec2 regionMin,
    glm::vec2 regionMax,
    CollisionDetector* collisionDetector,
    int curveId)
    : forwardParticle(startPosition),
    backwardParticle(startPosition),
    regionMin(regionMin),
    regionMax(regionMax),
    collisionDetector(collisionDetector),
    curveId(curveId),
    forwardForceDirection(validatedDirection(initialDirection)),
    backwardForceDirection(-validatedDirection(initialDirection)),
    forwardForceUpdateCounter(0),
    backwardForceUpdateCounter(0),
    forceUpdateInterval(400)
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

void CurveGrower::growBranch(
    Particle& particle,
    std::vector<glm::vec2>& points,
    bool& active,
    const ImageDirectionField& imageField,
    float deltaTime,
    CollisionResult& collisionResult,
    BranchStopReason& stopReason,
    glm::vec2& forceDirection,
    int& forceUpdateCounter)
{
    if (!active)
    {
        return;
    }

    // Periodically update the force direction from the image.
    if (forceUpdateCounter == 0)
    {
        forceDirection = imageField.getDirection(
            particle.getPosition(),
            forceDirection
        );
    }

    // Advance the force update timer.
    ++forceUpdateCounter;

    if (forceUpdateCounter >= forceUpdateInterval)
    {
        forceUpdateCounter = 0;
    }

    Particle candidate = particle;

    // The image tangent is the only source of force.
    candidate.update(forceDirection, deltaTime);

    glm::vec2 start = particle.getPosition();
    glm::vec2 end = candidate.getPosition();

    // Check whether the movement crosses the region boundary.
    bool hitBoundary = !isInsideRegion(end);

    glm::vec2 boundaryPosition(0.0f);

    if (hitBoundary)
    {
        boundaryPosition = clipToBoundary(start, end);
    }

    // Select the furthest position allowed by the region boundary.
    glm::vec2 target = end;

    if (hitBoundary)
    {
        target = boundaryPosition;
    }

    // Check the proposed movement against existing capsules.
    if (collisionDetector != nullptr && curveId >= 0)
    {
        CollisionResult result = collisionDetector->findCollision(
            start,
            target,
            1.0f,
            curveId
        );

        if (result.collided)
        {
            collisionResult = result;

            stopReason = BranchStopReason::EdgeCollision;

            if (result.hasEdge)
            {
                points.push_back(result.edgePosition);
            }
            else
            {
                points.push_back(result.safePosition);
            }

            active = false;

            std::cout << "Collision with edge "
                << result.edgeId
                << " at ("
                << result.edgePosition.x << ", "
                << result.edgePosition.y
                << ")\n";

            return;
        }
    }

    // No collision: handle the boundary or accept the full movement.
    if (hitBoundary)
    {
        points.push_back(boundaryPosition);

        stopReason = BranchStopReason::BoundaryReached;

        active = false;
    }
    else
    {
        particle = candidate;
        points.push_back(end);
    }
}

void CurveGrower::growStep(
    const ImageDirectionField& imageField,
    float deltaTime)
{
    growBranch(
        forwardParticle,
        forwardPoints,
        forwardActive,
        imageField,
        deltaTime,
        forwardCollision,
        forwardStopReason,
        forwardForceDirection,
        forwardForceUpdateCounter
    );

    growBranch(
        backwardParticle,
        backwardPoints,
        backwardActive,
        imageField,
        deltaTime,
        backwardCollision,
        backwardStopReason,
        backwardForceDirection,
        backwardForceUpdateCounter
    );
}

void CurveGrower::setForceUpdateInterval(int interval)
{
    if (interval <= 0)
    {
        throw std::invalid_argument(
            "Force update interval must be positive.");
    }

    forceUpdateInterval = interval;

    forwardForceUpdateCounter = 0;
    backwardForceUpdateCounter = 0;
}

const Curve& CurveGrower::getCurve() const
{
    return curve;
}

int CurveGrower::getCurveId() const
{
    return curveId;
}

const CollisionResult& CurveGrower::getForwardCollision() const
{
    return forwardCollision;
}

const CollisionResult& CurveGrower::getBackwardCollision() const
{
    return backwardCollision;
}

BranchStopReason CurveGrower::getForwardStopReason() const
{
    return forwardStopReason;
}

BranchStopReason CurveGrower::getBackwardStopReason() const
{
    return backwardStopReason;
}

bool CurveGrower::isFinished() const
{
    return !forwardActive && !backwardActive;
}

void CurveGrower::finalizeCurve()
{
    rebuildCurve();
}