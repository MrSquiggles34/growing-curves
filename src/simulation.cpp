#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

Simulation::Simulation(int width, int height)
    : width(width),
    height(height),
    directionField(width, height),
    distanceMap(width, height),
    occupancyGrid(width, height),
    nextCurveId(0),
    randomGenerator(std::random_device{}()),
    initialSpeed(20.0f),
    minimumDistance(10.0f),
    maximumCurves(10),
    finished(false),
    stopReason(StopReason::None),
    maximumRemainingDistance(0.0f),
    currentStartPosition(0.0f, 0.0f)
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument(
            "Simulation dimensions must be positive."
        );
    }

    startNextCurve();
}

void Simulation::step(float deltaTime)
{
    if (finished)
    {
        return;
    }

    if (!currentGrower)
    {
        if (!startNextCurve())
        {
            if (stopReason == StopReason::None)
            {
                stopReason = StopReason::NoValidStartingPoint;
            }

            finished = true;
            return;
        }
    }

    // Advance both branches of the current curve.
    currentGrower->growStep(
        directionField,
        deltaTime,
        curves
    );

    // Once both branches stop, save the curve and
    // prepare the next starting point.
    if (currentGrower->isFinished())
    {
        finishCurrentCurve();
    }
}

const std::vector<Curve>& Simulation::getCurves() const
{
    return curves;
}

const Curve* Simulation::getCurrentCurve() const
{
    if (!currentGrower)
    {
        return nullptr;
    }

    return &currentGrower->getCurve();
}

bool Simulation::isFinished() const
{
    return finished;
}

glm::vec2 Simulation::generateRandomPosition()
{
    std::uniform_real_distribution<float> xDistribution(
        0.0f,
        static_cast<float>(width - 1)
    );

    std::uniform_real_distribution<float> yDistribution(
        0.0f,
        static_cast<float>(height - 1)
    );

    return glm::vec2(
        xDistribution(randomGenerator),
        yDistribution(randomGenerator)
    );
}

glm::vec2 Simulation::generateRandomDirection()
{
    const float pi = 3.14159265358979323846f;

    std::uniform_real_distribution<float> angleDistribution(
        0.0f,
        2.0f * pi
    );

    float angle = angleDistribution(randomGenerator);

    return glm::vec2(
        std::cos(angle),
        std::sin(angle)
    );
}

bool Simulation::startNextCurve()
{
    if (static_cast<int>(curves.size()) >= maximumCurves)
    {
        stopReason = StopReason::MaximumCurvesReached;
        return false;
    }

    glm::vec2 startPosition;

    if (curves.empty())
    {
        // The first curve starts at a random position.
        startPosition = generateRandomPosition();
    }
    else
    {
        // Later curves start in the largest remaining gap.
        distanceMap.update(curves);

        startPosition = distanceMap.findFarthestPoint();

        maximumRemainingDistance = distanceMap.getDistance(
            static_cast<int>(startPosition.x),
            static_cast<int>(startPosition.y)
        );

        float remainingDistance = distanceMap.getDistance(
            static_cast<int>(startPosition.x),
            static_cast<int>(startPosition.y)
        );

        if (maximumRemainingDistance < minimumDistance)
        {
            stopReason = StopReason::MinimumDistanceReached;
            return false;
        }

        if (remainingDistance < minimumDistance)
        {
            return false;
        }
    }

    currentStartPosition = startPosition;

    glm::vec2 initialDirection = generateRandomDirection();

    currentGrower = std::make_unique<CurveGrower>(
        startPosition,
        initialDirection,
        initialSpeed,
        glm::vec2(0.0f, 0.0f),
        glm::vec2(
            static_cast<float>(width - 1),
            static_cast<float>(height - 1)
        ),
        &occupancyGrid,
        nextCurveId++
    );

    return true;
}

void Simulation::finishCurrentCurve()
{
    // Store the completed curve before destroying its grower.
    currentGrower->finalizeCurve();

    curves.push_back(currentGrower->getCurve());

    currentGrower.reset();

    // Stop if we have reached the curve limit or the
    // remaining gaps are too small.
    if (static_cast<int>(curves.size()) >= maximumCurves)
    {
        stopReason = StopReason::MaximumCurvesReached;
        finished = true;
        return;
    }

    if (!startNextCurve())
    {
        if (stopReason == StopReason::None)
        {
            stopReason = StopReason::NoValidStartingPoint;
        }

        finished = true;
    }
}


// Getters
Simulation::StopReason Simulation::getStopReason() const
{
    return stopReason;
}

float Simulation::getMaximumRemainingDistance() const
{
    return maximumRemainingDistance;
}

glm::vec2 Simulation::getCurrentStartPosition() const
{
    return currentStartPosition;
}