
#pragma once

#include "Curve.h"
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
        glm::vec2 regionMin = glm::vec2(0.0f, 0.0f),
        glm::vec2 regionMax = glm::vec2(255.0f, 255.0f),
        CollisionDetector* collisionDetector = nullptr,
        int curveId = -1);

    void growStep(
        const ImageDirectionField& imageField,
        float deltaTime);

    const Curve& getCurve() const;
    int getCurveId() const;

    const CollisionResult& getForwardCollision() const;
    const CollisionResult& getBackwardCollision() const;

    BranchStopReason getForwardStopReason() const;
    BranchStopReason getBackwardStopReason() const;

    bool isFinished() const;

    void setForceUpdateInterval(int interval);

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

    glm::vec2 forwardForceDirection;
    glm::vec2 backwardForceDirection;

    int forwardForceUpdateCounter;
    int backwardForceUpdateCounter;

    int forceUpdateInterval;

    bool isInsideRegion(glm::vec2 position) const;

    glm::vec2 clipToBoundary(
        glm::vec2 start,
        glm::vec2 end) const;


    void growBranch(
        Particle& particle,
        std::vector<glm::vec2>& points,
        bool& active,
        const ImageDirectionField& imageField,
        float deltaTime,
        CollisionResult& collisionResult,
        BranchStopReason& stopReason,
        glm::vec2& forceDirection,
        int& forceUpdateCounter);

    void rebuildCurve();
};


// Template implementation: growStep



