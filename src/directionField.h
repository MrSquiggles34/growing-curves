#pragma once

#include <glm/glm.hpp>
#include <vector>

class DirectionField
{
public:
    DirectionField(int width, int height);

    glm::vec2 getDirection(glm::vec2 position) const;

private:
    int width;
    int height;

    int gridSpacing;
    int gridWidth;
    int gridHeight;

    std::vector<glm::vec2> directions;

    glm::vec2 getGridDirection(int x, int y) const;
};