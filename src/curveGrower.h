
#pragma once

#include "Curve.h"
#include "DirectionField.h"
#include "Particle.h"
#include "occupancyGrid.h"

#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include <iostream>
#include <vector>

class CurveGrower
{
public:
    CurveGrower(
        glm::vec2 startPosition,
        glm::vec2 initialDirection,
        float initialSpeed = 1.0f,
        glm::vec2 regionMin = glm::vec2(0.0f, 0.0f),
        glm::vec2 regionMax = glm::vec2(255.0f, 255.0f),
        OccupancyGrid* occupancyGrid = nullptr,
        int curveId = -1);

    template <typename Field>
    void growStep(
        const Field& field,
        float deltaTime,
        const std::vector<Curve>& existingCurves);

    const Curve& getCurve() const;
    bool isFinished() const;

    void finalizeCurve();


private:
    Particle forwardParticle;
    Particle backwardParticle;

    std::vector<glm::vec2> forwardPoints;
    std::vector<glm::vec2> backwardPoints;

    Curve curve;

    glm::vec2 regionMin;
    glm::vec2 regionMax;

    OccupancyGrid* occupancyGrid;
    int curveId;

    bool forwardActive = true;
    bool backwardActive = true;

    bool isInsideRegion(glm::vec2 position) const;

    glm::vec2 clipToBoundary(
        glm::vec2 start,
        glm::vec2 end) const;

    bool findFirstIntersection(
        glm::vec2 start,
        glm::vec2 end,
        const std::vector<Curve>& existingCurves,
        glm::vec2& intersection,
        float& intersectionT) const;

    template <typename Field>
    void growBranch(
        Particle& particle,
        std::vector<glm::vec2>& points,
        bool& active,
        const Field& field,
        float deltaTime,
        const std::vector<Curve>& existingCurves);

    void rebuildCurve();
};


// Template implementation: growStep

template <typename Field>
void CurveGrower::growStep(
    const Field& field,
    float deltaTime,
    const std::vector<Curve>& existingCurves)
{
    growBranch(
        forwardParticle,
        forwardPoints,
        forwardActive,
        field,
        deltaTime,
        existingCurves);

    growBranch(
        backwardParticle,
        backwardPoints,
        backwardActive,
        field,
        deltaTime,
        existingCurves);
}


// Template implementation: growBranch
template <typename Field>
void CurveGrower::growBranch(
    Particle& particle,
    std::vector<glm::vec2>& points,
    bool& active,
    const Field& field,
    float deltaTime,
    const std::vector<Curve>& existingCurves)
{
    if (!active)
    {
        return;
    }

    // Predict the next position without committing it yet.
    Particle candidate = particle;

    glm::vec2 force =
        field.getVorticesDirection(candidate.getPosition());

    candidate.update(force, deltaTime);

    glm::vec2 start = particle.getPosition();
    glm::vec2 end = candidate.getPosition();

    // Check for intersection with an existing curve.
    /*
    glm::vec2 intersection(0.0f);
    float intersectionT = 1.0f;

    bool hitCurve = findFirstIntersection(
        start,
        end,
        existingCurves,
        intersection,
        intersectionT);
        */

    // Check whether the movement crosses the region boundary.
    bool hitBoundary = !isInsideRegion(end);

    float boundaryT = 1.0f;
    glm::vec2 boundaryPosition(0.0f);

    if (hitBoundary)
    {
        boundaryPosition = clipToBoundary(start, end);

        glm::vec2 movement = end - start;
        float movementLengthSquared =
            glm::dot(movement, movement);

        if (movementLengthSquared > 0.0f)
        {
            boundaryT = glm::dot(
                boundaryPosition - start,
                movement) / movementLengthSquared;
        }
    }

    // Select the first geometric stopping point.
    glm::vec2 target = end;

    if (hitBoundary)
    {
        target = boundaryPosition;
    }

    // Check every sampled cell between the current position
    // and the selected target.
    if (occupancyGrid != nullptr && curveId >= 0)
    {
        glm::vec2 lastValidPosition;

        bool hitOccupiedCell =
            occupancyGrid->findFirstBlockedPoint(
                start,
                target,
                curveId,
                lastValidPosition);

        if (hitOccupiedCell)
        {
            // Claim the valid portion of this segment.
            occupancyGrid->occupySegment(
                start,
                lastValidPosition,
                curveId);

            points.push_back(lastValidPosition);
            active = false;

            std::cout << "Occupancy collision from ("
                << start.x << ", " << start.y
                << ") before ("
                << target.x << ", " << target.y
                << ")\n";

            return;
        }
    }

    // Register the accepted portion of this movement.
    if (occupancyGrid != nullptr && curveId >= 0)
    {
        occupancyGrid->occupySegment(
            start,
            target,
            curveId);
    }

    if (hitBoundary)
    {
        points.push_back(boundaryPosition);
        active = false;
    }
    else
    {
        particle = candidate;
        points.push_back(end);
    }
}