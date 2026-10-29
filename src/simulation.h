#pragma once

#include "curve.h"
#include "curveGrower.h"
#include "directionField.h"
#include "distanceMap.h"
#include "occupancyGrid.h"

#include <glm/vec2.hpp>
#include <memory>
#include <random>
#include <vector>

class Simulation
{
public:
    Simulation(int width, int height);

    enum class StopReason
    {
        None,
        MinimumDistanceReached,
        MaximumCurvesReached,
        NoValidStartingPoint
    }; 

    void step(float deltaTime);

    const std::vector<Curve>& getCurves() const;
    const Curve* getCurrentCurve() const;

    bool isFinished() const;

    StopReason getStopReason() const;
    float getMaximumRemainingDistance() const;
    glm::vec2 getCurrentStartPosition() const;

private:
    int width;
    int height;

    DirectionField directionField;
    DistanceMap distanceMap;

    // Occupancy grid
    OccupancyGrid occupancyGrid;
    int nextCurveId;

    std::vector<Curve> curves;
    std::unique_ptr<CurveGrower> currentGrower;

    std::mt19937 randomGenerator;

    float initialSpeed;
    float minimumDistance;
    int maximumCurves;

    bool finished;

    glm::vec2 generateRandomPosition();
    glm::vec2 generateRandomDirection();

    bool startNextCurve();
    void finishCurrentCurve();

	// Stop reason tracking
    StopReason stopReason;
    float maximumRemainingDistance;
    glm::vec2 currentStartPosition;


};
