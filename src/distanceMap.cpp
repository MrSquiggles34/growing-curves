#include "distanceMap.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <chrono>
#include <iostream>

#include <glm/geometric.hpp>

DistanceMap::DistanceMap(int width, int height)
    : width(width),
    height(height),
    distances(width* height, 0.0f)
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument(
            "DistanceMap dimensions must be positive.");
    }

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            distances[y * width + x] = distanceToBoundary(x, y);
        }
    }
}

float DistanceMap::distanceToBoundary(int x, int y) const
{
    float left = static_cast<float>(x);
    float top = static_cast<float>(y);
    float right = static_cast<float>(width - 1 - x);
    float bottom = static_cast<float>(height - 1 - y);

    return std::min(
        std::min(left, right),
        std::min(top, bottom));
}

float DistanceMap::distanceToCurve(
    glm::vec2 point,
    const Curve& curve) const
{
    if (curve.points.empty())
    {
        return std::numeric_limits<float>::max();
    }

    if (curve.points.size() == 1)
    {
        return glm::distance(point, curve.points[0]);
    }

    float minimumDistanceSquared =
        std::numeric_limits<float>::max();

    for (std::size_t i = 1; i < curve.points.size(); ++i)
    {
        glm::vec2 start = curve.points[i - 1];
        glm::vec2 end = curve.points[i];

        glm::vec2 segment = end - start;
        float segmentLengthSquared =
            glm::dot(segment, segment);

        float t = 0.0f;

        if (segmentLengthSquared > 0.0f)
        {
            t = glm::dot(point - start, segment)
                / segmentLengthSquared;
        }

        t = std::clamp(t, 0.0f, 1.0f);

        glm::vec2 closestPoint = start + t * segment;
        glm::vec2 difference = point - closestPoint;

        float distanceSquared =
            glm::dot(difference, difference);

        minimumDistanceSquared = std::min(
            minimumDistanceSquared,
            distanceSquared);
    }

    return std::sqrt(minimumDistanceSquared);
}

void DistanceMap::update(const std::vector<Curve>& curves)
{
    auto startTime = std::chrono::steady_clock::now();

    for (std::size_t curveIndex = processedCurveCount;
        curveIndex < curves.size();
        ++curveIndex)
    {
        const Curve& curve = curves[curveIndex];

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                glm::vec2 point(
                    static_cast<float>(x),
                    static_cast<float>(y));

                float curveDistance = distanceToCurve(point, curve);

                int index = y * width + x;

                distances[index] = std::min(
                    distances[index],
                    curveDistance);
            }
        }
    }

    processedCurveCount = curves.size();

    auto endTime = std::chrono::steady_clock::now();

    double elapsedMilliseconds =
        std::chrono::duration<double, std::milli>(
            endTime - startTime
        ).count();

    std::cout << "DistanceMap update: "
        << elapsedMilliseconds
        << " ms; curves processed: "
        << processedCurveCount
        << std::endl;
}

glm::vec2 DistanceMap::findFarthestPoint() const
{
    int bestX = 0;
    int bestY = 0;
    float maximumDistance = -1.0f;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float distance = distances[y * width + x];

            if (distance > maximumDistance)
            {
                maximumDistance = distance;
                bestX = x;
                bestY = y;
            }
        }
    }

    return glm::vec2(
        static_cast<float>(bestX),
        static_cast<float>(bestY));
}

float DistanceMap::getDistance(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        throw std::out_of_range(
            "DistanceMap coordinates are out of bounds.");
    }

    return distances[y * width + x];
}