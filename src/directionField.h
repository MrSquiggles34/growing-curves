#pragma once

#include <glm/vec2.hpp>

class DirectionField
{
public:
    explicit DirectionField(glm::vec2 center);

    glm::vec2 getDirection(glm::vec2 position) const;

private:
    glm::vec2 center;
};
