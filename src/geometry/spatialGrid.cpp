
#include "spatialGrid.h"

#include <algorithm>
#include <cmath>

SpatialGrid::SpatialGrid(int width, int height, float cellSize)
    : width(width),
    height(height),
    cellSize(cellSize)
{
    columns = static_cast<int>(std::ceil(width / cellSize));
    rows = static_cast<int>(std::ceil(height / cellSize));

    cells.resize(columns * rows);
}

int SpatialGrid::getCellX(float x) const
{
    return static_cast<int>(std::floor(x / cellSize));
}

int SpatialGrid::getCellY(float y) const
{
    return static_cast<int>(std::floor(y / cellSize));
}

int SpatialGrid::getCellIndex(int x, int y) const
{
    return y * columns + x;
}

void SpatialGrid::insertEdge(
    const GeometryNetwork& network,
    EdgeId edgeId)
{
    const Edge& edge = network.getEdge(edgeId);

    const glm::vec2& start =
        network.getVertex(edge.startVertex).position;

    const glm::vec2& end =
        network.getVertex(edge.endVertex).position;

    float minX = std::min(start.x, end.x) - edge.radius;
    float maxX = std::max(start.x, end.x) + edge.radius;

    float minY = std::min(start.y, end.y) - edge.radius;
    float maxY = std::max(start.y, end.y) + edge.radius;

    int minCellX = std::max(0, getCellX(minX));
    int maxCellX = std::min(columns - 1, getCellX(maxX));

    int minCellY = std::max(0, getCellY(minY));
    int maxCellY = std::min(rows - 1, getCellY(maxY));

    for (int y = minCellY; y <= maxCellY; y++)
    {
        for (int x = minCellX; x <= maxCellX; x++)
        {
            cells[getCellIndex(x, y)].push_back(edgeId);
        }
    }
}

std::vector<EdgeId> SpatialGrid::getCandidateEdges(
    const glm::vec2& start,
    const glm::vec2& end,
    float radius) const
{
    float minX = std::min(start.x, end.x) - radius;
    float maxX = std::max(start.x, end.x) + radius;

    float minY = std::min(start.y, end.y) - radius;
    float maxY = std::max(start.y, end.y) + radius;

    int minCellX = std::max(0, getCellX(minX));
    int maxCellX = std::min(columns - 1, getCellX(maxX));

    int minCellY = std::max(0, getCellY(minY));
    int maxCellY = std::min(rows - 1, getCellY(maxY));

    std::vector<EdgeId> candidates;

    // A movement completely outside the grid cannot find any edges.
    if (minCellX > maxCellX || minCellY > maxCellY)
    {
        return candidates;
    }

    for (int y = minCellY; y <= maxCellY; y++)
    {
        for (int x = minCellX; x <= maxCellX; x++)
        {
            const std::vector<EdgeId>& cell =
                cells[getCellIndex(x, y)];

            candidates.insert(
                candidates.end(),
                cell.begin(),
                cell.end()
            );
        }
    }

    // An edge can appear in multiple cells.
    // We only need to test it once.
    std::sort(candidates.begin(), candidates.end());

    candidates.erase(
        std::unique(candidates.begin(), candidates.end()),
        candidates.end()
    );

    return candidates;
}
