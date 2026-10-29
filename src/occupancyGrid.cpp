#include "occupancyGrid.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

OccupancyGrid::OccupancyGrid(int width, int height, int cellSize)
    : width(width),
    height(height),
    cellSize(cellSize),
    gridWidth((width + cellSize - 1) / cellSize),
    gridHeight((height + cellSize - 1) / cellSize)
{
    if (width <= 0 || height <= 0 || cellSize <= 0)
    {
        throw std::invalid_argument("Invalid occupancy grid dimensions");
    }

    cells.assign(gridWidth * gridHeight, -1);
}

void OccupancyGrid::worldToGrid(
    int x,
    int y,
    int& gridX,
    int& gridY) const
{
    gridX = x / cellSize;
    gridY = y / cellSize;
}

bool OccupancyGrid::isInside(int x, int y) const
{
    return x >= 0 && x < gridWidth &&
        y >= 0 && y < gridHeight;
}

int OccupancyGrid::getIndex(int x, int y) const
{
    return y * gridWidth + x;
}

int OccupancyGrid::getOwner(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return -2;
    }

    int gridX;
    int gridY;

    worldToGrid(x, y, gridX, gridY);

    return cells[getIndex(gridX, gridY)];
}

bool OccupancyGrid::canOccupy(int x, int y, int curveId) const
{
    int owner = getOwner(x, y);

    return owner == -1 || owner == curveId;
}

bool OccupancyGrid::occupy(int x, int y, int curveId)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return false;
    }

    int gridX;
    int gridY;

    worldToGrid(x, y, gridX, gridY);

    int index = getIndex(gridX, gridY);

    if (cells[index] != -1 && cells[index] != curveId)
    {
        return false;
    }

    cells[index] = curveId;
    return true;
}

void OccupancyGrid::occupySegment(
    glm::vec2 start,
    glm::vec2 end,
    int curveId
)
{
    glm::vec2 difference = end - start;
    float length = glm::length(difference);

    // Sample at least twice per pixel of segment length.
    int steps = std::max(
        1,
        static_cast<int>(std::ceil(glm::length(end - start) * 2.0f))
    );

    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec2 point = start + difference * t;

        int x = static_cast<int>(std::round(point.x));
        int y = static_cast<int>(std::round(point.y));

        // Existing curves should already be valid and inside.
        // Do not overwrite a different curve's ownership.
        occupy(x, y, curveId);
    }
}


bool OccupancyGrid::findFirstBlockedPoint(
    glm::vec2 start,
    glm::vec2 end,
    int curveId,
    glm::vec2& lastValidPosition) const
{
    glm::vec2 difference = end - start;

    float length = glm::length(difference);

    int steps = std::max(
        1,
        static_cast<int>(std::ceil(length * 2.0f))
    );

    lastValidPosition = start;

    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec2 point = start + difference * t;

        int x = static_cast<int>(std::round(point.x));
        int y = static_cast<int>(std::round(point.y));

        int owner = getOwner(x, y);

        if (owner != -1 && owner != curveId)
        {
            return true;
        }

        lastValidPosition = point;
    }

    return false;
}
