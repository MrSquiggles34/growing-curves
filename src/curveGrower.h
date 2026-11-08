
#pragma once

#include "Curve.h"
#include "DirectionField.h"
#include "Particle.h"
#include "geometry/collisionDetector.h"

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
        CollisionDetector* collisionDetector = nullptr,
        int curveId = -1);

    template <typename Field>
    void growStep(
        const Field& field,
        float deltaTime);

    const Curve& getCurve() const;
    int getCurveId() const;

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
        float deltaTime);

    void rebuildCurve();
};


// Template implementation: growStep

template <typename Field>
void CurveGrower::growStep(
    const Field& field,
    float deltaTime)
{
    growBranch(
        forwardParticle,
        forwardPoints,
        forwardActive,
        field,
        deltaTime);

    growBranch(
        backwardParticle,
        backwardPoints,
        backwardActive,
        field,
        deltaTime);
}


// Template implementation: growBranch
template <typename Field>
void CurveGrower::growBranch(
    Particle& particle,
    std::vector<glm::vec2>& points,
    bool& active,
    const Field& field,
    float deltaTime)
{
    if (!active)
    {
        return;
    }

    Particle candidate = particle;

    glm::vec2 force =
        field.getVorticesDirection(candidate.getPosition());

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
        bool hitCollision = collisionDetector->wouldCollide(
            start,
            target,
            1.0f,
            curveId
        );

        if (hitCollision)
        {
            glm::vec2 lastSafePosition =
                collisionDetector->findLastSafePosition(
                    start,
                    target,
                    1.0f,
                    curveId
                );

            // Only append a point if the branch actually moved.
            if (glm::distance(start, lastSafePosition) > 0.0001f)
            {
                points.push_back(lastSafePosition);
            }

            active = false;

            std::cout << "Capsule collision from ("
                << start.x << ", " << start.y
                << ") before ("
                << target.x << ", " << target.y
                << ")\n";

            return;
        }
    }

    // No collision: handle the boundary or accept the full movement.
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