#pragma once

#include <glm/vec2.hpp>

class StraightDirectionField
{
public:
    glm::vec2 getDirection(glm::vec2 position) const
    {
        return glm::vec2(0.0f, 0.0f);
    }
};