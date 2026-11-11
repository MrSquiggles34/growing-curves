#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace
{
    VertexId registerCollision(
        GeometryNetwork& network,
        SpatialGrid& grid,
        const CollisionResult& collision)
    {
        const Edge& edge = network.getEdge(collision.edgeId);

        const float epsilon = 0.0001f;

        // If the collision is already at an endpoint,
        // reuse that vertex instead of splitting the edge.
        if (collision.edgeParameter <= epsilon)
        {
            return edge.startVertex;
        }

        if (collision.edgeParameter >= 1.0f - epsilon)
        {
            return edge.endVertex;
        }

        // Split the existing edge at the collision position.
        VertexId newVertex = network.splitEdge(
            collision.edgeId,
            collision.edgePosition
        );

        // The original edge has changed, and a new edge was added.
        // Register both with the spatial grid.
        grid.insertEdge(network, collision.edgeId);

        EdgeId newEdgeId = network.getEdgeCount() - 1;

        grid.insertEdge(network, newEdgeId);

        return newVertex;
    }
}

Simulation::Simulation(int width, int height)
    : width(width),
    height(height),
    directionField(width, height),
    imageDirectionField(),
    distanceMap(width, height),
    imageInfluence(1.0f),
    geometryNetwork(),
    spatialGrid(width, height, 2.0f),
    collisionDetector(geometryNetwork, spatialGrid),
    nextCurveId(0),
    randomGenerator(std::random_device{}()),
    initialSpeed(9.0f),
    minimumDistance(1.0f),
    maximumCurves(100),
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

    // Load the image used to guide particle movement.
    if (!imageDirectionField.loadImage("test.png"))
    {
        std::cout << "Warning: Image guidance disabled."
            << std::endl;
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
        &imageDirectionField,
        imageInfluence
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

const GeometryNetwork& Simulation::getGeometryNetwork() const
{
    return geometryNetwork;
}

const CollisionDetector& Simulation::getCollisionDetector() const
{
	return collisionDetector;
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
        &collisionDetector,
        nextCurveId++
    );

    return true;
}

void Simulation::finishCurrentCurve()
{
    currentGrower->finalizeCurve();

    const Curve& completedCurve = currentGrower->getCurve();

    if (completedCurve.points.size() < 2)
    {
        std::cout << "Rejected curve: insufficient points."
            << std::endl;

        currentGrower.reset();

        if (!startNextCurve())
        {
            if (stopReason == StopReason::None)
            {
                stopReason = StopReason::NoValidStartingPoint;
            }

            finished = true;
        }

        return;
    }

    const CollisionResult& forwardCollision =
        currentGrower->getForwardCollision();

    const CollisionResult& backwardCollision =
        currentGrower->getBackwardCollision();

    bool hasForwardCollision =
        forwardCollision.collided && forwardCollision.hasEdge;

    bool hasBackwardCollision =
        backwardCollision.collided && backwardCollision.hasEdge;


    bool useStartVertex = false;
    bool useEndVertex = false;

    VertexId startVertex = 0;
    VertexId endVertex = 0;

    VertexId forwardVertex = 0;
    VertexId backwardVertex = 0;

    // Resolve collision vertices.
    if (hasForwardCollision && hasBackwardCollision)
    {
        if (forwardCollision.edgeId == backwardCollision.edgeId)
        {
            float distance = glm::distance(
                forwardCollision.edgePosition,
                backwardCollision.edgePosition
            );

            if (distance < 0.0001f)
            {
                // Both branches reached the same location.
                forwardVertex = registerCollision(
                    geometryNetwork,
                    spatialGrid,
                    forwardCollision
                );

                backwardVertex = forwardVertex;
            }
            else if (forwardCollision.edgeParameter >
                backwardCollision.edgeParameter)
            {
                forwardVertex = registerCollision(
                    geometryNetwork,
                    spatialGrid,
                    forwardCollision
                );

                backwardVertex = registerCollision(
                    geometryNetwork,
                    spatialGrid,
                    backwardCollision
                );
            }
            else
            {
                backwardVertex = registerCollision(
                    geometryNetwork,
                    spatialGrid,
                    backwardCollision
                );

                forwardVertex = registerCollision(
                    geometryNetwork,
                    spatialGrid,
                    forwardCollision
                );
            }
        }
        else
        {
            forwardVertex = registerCollision(
                geometryNetwork,
                spatialGrid,
                forwardCollision
            );

            backwardVertex = registerCollision(
                geometryNetwork,
                spatialGrid,
                backwardCollision
            );
        }
    }
    else if (hasForwardCollision)
    {
        forwardVertex = registerCollision(
            geometryNetwork,
            spatialGrid,
            forwardCollision
        );
    }
    else if (hasBackwardCollision)
    {
        backwardVertex = registerCollision(
            geometryNetwork,
            spatialGrid,
            backwardCollision
        );
    }

    // Determine which endpoint belongs to each collision.
    if (hasForwardCollision)
    {
        float startDistance = glm::distance(
            completedCurve.points.front(),
            forwardCollision.edgePosition
        );

        float endDistance = glm::distance(
            completedCurve.points.back(),
            forwardCollision.edgePosition
        );

        if (startDistance < endDistance)
        {
            useStartVertex = true;
            startVertex = forwardVertex;
        }
        else
        {
            useEndVertex = true;
            endVertex = forwardVertex;
        }
    }

    if (hasBackwardCollision)
    {
        float startDistance = glm::distance(
            completedCurve.points.front(),
            backwardCollision.edgePosition
        );

        float endDistance = glm::distance(
            completedCurve.points.back(),
            backwardCollision.edgePosition
        );

        if (startDistance < endDistance)
        {
            useStartVertex = true;
            startVertex = backwardVertex;
        }
        else
        {
            useEndVertex = true;
            endVertex = backwardVertex;
        }
    }

    if (hasForwardCollision || hasBackwardCollision)
    {
        std::cout << "Curve connected to existing geometry." << std::endl;
    }

    // Register the curve with its existing collision vertices.
    EdgeId firstNewEdge = geometryNetwork.getEdgeCount();

    geometryNetwork.addPolylineWithEndpoints(
        completedCurve.points,
        currentGrower->getCurveId(),
        useStartVertex,
        startVertex,
        useEndVertex,
        endVertex
    );

    // Register the new curve edges in the spatial grid.
    EdgeId lastNewEdge = geometryNetwork.getEdgeCount();

    for (EdgeId edgeId = firstNewEdge;
        edgeId < lastNewEdge;
        edgeId++)
    {
        spatialGrid.insertEdge(geometryNetwork, edgeId);
    }

    curves.push_back(completedCurve);
    currentGrower.reset();

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

