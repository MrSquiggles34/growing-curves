#include "DirectionField.h"

#include <algorithm>
#include <cmath>
#include <random>

DirectionField::DirectionField(int width, int height)
    : width(width),
    height(height),
    gridSpacing(32),
    gridWidth((width + gridSpacing - 1) / gridSpacing + 1),
    gridHeight((height + gridSpacing - 1) / gridSpacing + 1)
{
    std::mt19937 randomGenerator(std::random_device{}());
    std::uniform_real_distribution<float> randomAngle(
        0.0f,
        6.28318530718f
    );

    directions.reserve(gridWidth * gridHeight);

    for (int y = 0; y < gridHeight; ++y)
    {
        for (int x = 0; x < gridWidth; ++x)
        {
            float angle = randomAngle(randomGenerator);

            directions.push_back(glm::vec2(
                std::cos(angle),
                std::sin(angle)
            ));
        }
    }
}

glm::vec2 DirectionField::getGridDirection(int x, int y) const
{
    x = std::clamp(x, 0, gridWidth - 1);
    y = std::clamp(y, 0, gridHeight - 1);

    return directions[y * gridWidth + x];
}

glm::vec2 DirectionField::getRandomDirection(glm::vec2 position) const
{
    float gridX = position.x / gridSpacing;
    float gridY = position.y / gridSpacing;

    int x0 = static_cast<int>(std::floor(gridX));
    int y0 = static_cast<int>(std::floor(gridY));

    float tx = gridX - x0;
    float ty = gridY - y0;

    glm::vec2 topLeft = getGridDirection(x0, y0);
    glm::vec2 topRight = getGridDirection(x0 + 1, y0);
    glm::vec2 bottomLeft = getGridDirection(x0, y0 + 1);
    glm::vec2 bottomRight = getGridDirection(x0 + 1, y0 + 1);

    glm::vec2 top = topLeft * (1.0f - tx) + topRight * tx;
    glm::vec2 bottom = bottomLeft * (1.0f - tx) + bottomRight * tx;

    glm::vec2 direction = top * (1.0f - ty) + bottom * ty;

    float magnitude = glm::length(direction);

    if (magnitude > 0.0001f)
    {
        direction /= magnitude;
    }
    else
    {
        direction = topLeft;
    }

    return direction;
}


glm::vec2 DirectionField::getVortexDirection(glm::vec2 position) const
{
    // Center of the vortex
    glm::vec2 center(width * 0.5f, height * 0.5f);

    glm::vec2 offset = position - center;

    // Counterclockwise tangent
    glm::vec2 direction(offset.y, -offset.x);

    float magnitude = glm::length(direction);

    if (magnitude > 0.0001f)
    {
        direction /= magnitude;
    }
    else
    {
        direction = glm::vec2(0.0f, -1.0f);
    }

    return direction;
}




glm::vec2 DirectionField::getVorticesDirection(glm::vec2 position) const
{
    const int columns = 4;
    const int rows = 3;

    float cellWidth = static_cast<float>(width) / columns;
    float cellHeight = static_cast<float>(height) / rows;

    float closestDistSq = std::numeric_limits<float>::max();
    glm::vec2 closestCenter(0.0f);

    // Find the nearest vortex center.
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < columns; ++x)
        {
            glm::vec2 center(
                (x + 0.5f) * cellWidth,
                (y + 0.5f) * cellHeight
            );

            glm::vec2 offset = position - center;
            float distSq = glm::dot(offset, offset);

            if (distSq < closestDistSq)
            {
                closestDistSq = distSq;
                closestCenter = center;
            }
        }
    }

    // Counterclockwise direction around the nearest center.
    glm::vec2 offset = position - closestCenter;
    glm::vec2 direction(offset.y, -offset.x);

    float magnitude = glm::length(direction);

    if (magnitude > 0.0001f)
        direction /= magnitude;
    else
        direction = glm::vec2(0.0f, -1.0f);

    return direction;
}

