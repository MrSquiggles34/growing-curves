#pragma once

#include <glm/glm.hpp>
#include <vector>

class OccupancyGrid
{
public:
    OccupancyGrid(int width, int height, int cellSize = 1);

    int getOwner(int x, int y) const;
    bool canOccupy(int x, int y, int curveId) const;
    bool occupy(int x, int y, int curveId);

    // Mark the cells belonging to an existing curve.
    void occupySegment(
        glm::vec2 start,
        glm::vec2 end,
        int curveId
    );

    bool findFirstBlockedPoint(
        glm::vec2 start,
        glm::vec2 end,
        int curveId,
        glm::vec2& lastValidPosition) const;

private:
    int width;
    int height;
    int cellSize;

    int gridWidth;
    int gridHeight;

    std::vector<int> cells;

    bool isInside(int x, int y) const;
    int getIndex(int x, int y) const;

    void worldToGrid(int x, int y, int& gridX, int& gridY) const;
};