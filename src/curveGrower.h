
#pragma once

#include "Curve.h"
#include "DirectionField.h"
#include "Particle.h"
#include "geometry/collisionDetector.h"
#include "imageDirectionField.h"

#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include <iostream>
#include <vector>

enum class BranchStopReason
{
    None,
    EdgeCollision,
    BoundaryReached
};

class CurveGrower
{
public:
    CurveGrower(
        glm::vec2 startPosition,
        glm::vec2 initialDirection,
        float initialSpeed = 1.0f,
        glm::vec2 regionMin = glm::vec2(0.0f, 0.0f),
        glm::vec2 regionMax = glm::vec2(255.0f, 255.0f),
        CollisionDetector* collisionDetector = nullptr,
        int curveId = -1);

    template <typename Field>
    void growStep(
        const Field& field,
        float deltaTime,
        const ImageDirectionField* imageField = nullptr,
        float imageInfluence = 0.5f);

    const Curve& getCurve() const;
    int getCurveId() const;

    const CollisionResult& getForwardCollision() const;
    const CollisionResult& getBackwardCollision() const;

    BranchStopReason getForwardStopReason() const;
    BranchStopReason getBackwardStopReason() const;

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

    CollisionDetector* collisionDetector;
    int curveId;

    CollisionResult forwardCollision;
    CollisionResult backwardCollision;

    BranchStopReason forwardStopReason = BranchStopReason::None;
    BranchStopReason backwardStopReason = BranchStopReason::None;

    bool forwardActive = true;
    bool backwardActive = true;

    bool isInsideRegion(glm::vec2 position) const;

    glm::vec2 clipToBoundary(
        glm::vec2 start,
        glm::vec2 end) const;


    template <typename Field>
    void growBranch(
        Particle& particle,
        std::vector<glm::vec2>& points,
        bool& active,
        const Field& field,
        float deltaTime,
        CollisionResult& collisionResult,
        BranchStopReason& stopReason,
        const ImageDirectionField* imageField,
        float imageInfluence);

    void rebuildCurve();
};


// Template implementation: growStep

template <typename Field>
void CurveGrower::growStep(
    const Field& field,
    float deltaTime,
    const ImageDirectionField* imageField,
    float imageInfluence)
{
    growBranch(
        forwardParticle,
        forwardPoints,
        forwardActive,
        field,
        deltaTime,
        forwardCollision,
        forwardStopReason,
        imageField,
        imageInfluence
    );

    growBranch(
        backwardParticle,
        backwardPoints,
        backwardActive,
        field,
        deltaTime,
        backwardCollision,
        backwardStopReason,
        imageField,
        imageInfluence
    );
}


// Template implementation: growBranch
template <typename Field>
void CurveGrower::growBranch(
    Particle& particle,
    std::vector<glm::vec2>& points,
    bool& active,
    const Field& field,
    float deltaTime,
    CollisionResult& collisionResult,
    BranchStopReason& stopReason,
    const ImageDirectionField* imageField,
    float imageInfluence)
{
    if (!active)
    {
        return;
    }

    Particle candidate = particle;

    glm::vec2 simulationDirection =
        field.getVorticesDirection(candidate.getPosition());

    glm::vec2 force = simulationDirection;

    if (imageField != nullptr)
    {
        force = imageField->blendDirection(
            simulationDirection,
            candidate.getPosition(),
            imageInfluence
        );
    }

    candidate.update(force, deltaTime);

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