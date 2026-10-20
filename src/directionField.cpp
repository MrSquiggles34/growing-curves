#include "DirectionField.h"

#include <glm/geometric.hpp>

DirectionField::DirectionField(glm::vec2 center) : center(center) { }

glm::vec2 DirectionField::getDirection(glm::vec2 position) const
{
    glm::vec2 radial = position - center;

    glm::vec2 direction(-radial.y, radial.x);

    float magnitude = glm::length(direction);

    if (magnitude > 0.0f)
    {
        direction /= magnitude;
    }

    return direction;
}
