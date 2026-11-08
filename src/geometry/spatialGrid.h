
#pragma once

#include "geometryNetwork.h"

#include <glm/vec2.hpp>

#include <vector>

class SpatialGrid
{
public:
    SpatialGrid(int width, int height, float cellSize = 8.0f);

    void insertEdge(const GeometryNetwork& network, EdgeId edgeId);

    std::vector<EdgeId> getCandidateEdges(
        const glm::vec2& start,
        const glm::vec2& end,
        float radius
    ) const;

private:
    int columns;
    int rows;

    int width;
    int height;

    float cellSize;

    std::vector<std::vector<EdgeId>> cells;

    int getCellX(float x) const;
    int getCellY(float y) const;

    int getCellIndex(int x, int y) const;
};
