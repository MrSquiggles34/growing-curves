#pragma once

#include "curve.h"

#include <glm/vec2.hpp>
#include <vector>
#include <cstddef>

class DistanceMap
{
public:
    DistanceMap(int width, int height);

    void update(const std::vector<Curve>& curves);

    glm::vec2 findFarthestPoint() const;

    float getDistance(int x, int y) const;

private:
    int width;
    int height;

    std::size_t processedCurveCount = 0;

    std::vector<float> distances;

    float distanceToBoundary(int x, int y) const;

    float distanceToCurve(
        glm::vec2 point,
        const Curve& curve) const;
};