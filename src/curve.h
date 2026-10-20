
#pragma once

#include <glm/vec2.hpp>
#include <vector>

struct Curve
{
    std::vector<glm::vec2> points;
    float length = 0.0f;
};
