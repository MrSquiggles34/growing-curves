#include "distanceMap.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <chrono>
#include <iostream>

#include <glm/geometric.hpp>


namespace
{
    const float INF_DISTANCE = 1.0e20f;

    // Computes the squared distance transform of a 1D array.
    // Feature locations have finite input values; other locations
    // can be very large.
    void distanceTransform1D(
        const std::vector<float>& input,
        std::vector<float>& output)
    {
        int n = static_cast<int>(input.size());

        std::vector<int> sites(n);
        std::vector<float> boundaries(n + 1);

        int firstSite = -1;

        for (int i = 0; i < n; ++i)
        {
            if (input[i] < INF_DISTANCE * 0.5f)
            {
                firstSite = i;
                break;
            }
        }

        if (firstSite == -1)
        {
            std::fill(output.begin(), output.end(), INF_DISTANCE);
            return;
        }

        int k = 0;
        sites[0] = firstSite;
        boundaries[0] = -INF_DISTANCE;
        boundaries[1] = INF_DISTANCE;

        for (int q = firstSite + 1; q < n; ++q)
        {
            if (input[q] >= INF_DISTANCE * 0.5f)
            {
                continue;
            }

            float intersection = 0.0f;

            while (true)
            {
                int p = sites[k];

                intersection =
                    ((input[q] + static_cast<float>(q) * q)
                        - (input[p] + static_cast<float>(p) * p))
                    / (2.0f * (q - p));

                if (intersection > boundaries[k])
                {
                    break;
                }

                --k;
            }

            ++k;
            sites[k] = q;
            boundaries[k] = intersection;
            boundaries[k + 1] = INF_DISTANCE;
        }

        k = 0;

        for (int q = 0; q < n; ++q)
        {
            while (boundaries[k + 1] < q)
            {
                ++k;
            }

            float difference =
                static_cast<float>(q - sites[k]);

            output[q] =
                difference * difference + input[sites[k]];
        }
    }

    void rasterizeSegment(
        std::vector<float>& image,
        int width,
        int height,
        glm::vec2 start,
        glm::vec2 end)
    {
        int x0 = static_cast<int>(std::round(start.x));
        int y0 = static_cast<int>(std::round(start.y));
        int x1 = static_cast<int>(std::round(end.x));
        int y1 = static_cast<int>(std::round(end.y));

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);

        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        int error = dx - dy;

        while (true)
        {
            if (x0 >= 0 && x0 < width &&
                y0 >= 0 && y0 < height)
            {
                image[y0 * width + x0] = 0.0f;
            }

            if (x0 == x1 && y0 == y1)
            {
                break;
            }

            int twiceError = 2 * error;

            if (twiceError > -dy)
            {
                error -= dy;
                x0 += sx;
            }

            if (twiceError < dx)
            {
                error += dx;
                y0 += sy;
            }
        }
    }
}


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
    const int pixelCount = width * height;

    for (std::size_t curveIndex = processedCurveCount;
        curveIndex < curves.size();
        ++curveIndex)
    {
        const Curve& curve = curves[curveIndex];

        // Step 1: Rasterize this curve into a feature image.
        std::vector<float> distanceSquared(
            pixelCount,
            INF_DISTANCE);

        if (curve.points.size() == 1)
        {
            glm::vec2 point = curve.points[0];

            rasterizeSegment(
                distanceSquared,
                width,
                height,
                point,
                point);
        }
        else
        {
            for (std::size_t i = 1;
                i < curve.points.size();
                ++i)
            {
                rasterizeSegment(
                    distanceSquared,
                    width,
                    height,
                    curve.points[i - 1],
                    curve.points[i]);
            }
        }

        // Step 2: Transform every row.
        std::vector<float> input(width);
        std::vector<float> output(width);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                input[x] = distanceSquared[y * width + x];
            }

            distanceTransform1D(input, output);

            for (int x = 0; x < width; ++x)
            {
                distanceSquared[y * width + x] = output[x];
            }
        }

        // Step 3: Transform every column.
        input.resize(height);
        output.resize(height);

        for (int x = 0; x < width; ++x)
        {
            for (int y = 0; y < height; ++y)
            {
                input[y] = distanceSquared[y * width + x];
            }

            distanceTransform1D(input, output);

            for (int y = 0; y < height; ++y)
            {
                distanceSquared[y * width + x] = output[y];
            }
        }

        // Step 4: Merge the new curve's distances into our map.
        for (int i = 0; i < pixelCount; ++i)
        {
            float curveDistance =
                std::sqrt(distanceSquared[i]);

            distances[i] = std::min(
                distances[i],
                curveDistance);
        }
    }

    processedCurveCount = curves.size();
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