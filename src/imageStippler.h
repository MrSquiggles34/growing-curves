#pragma once

#include "imageDirectionField.h"
#include "distanceMap.h"

#include <glm/vec2.hpp>
#include <random>

class ImageStippler
{
public:
    ImageStippler(float minimumWeight = 0.25f);

    bool selectPosition(
        const ImageDirectionField& imageField,
        const DistanceMap& distanceMap,
        float minimumDistance,
        std::mt19937& randomGenerator,
        glm::vec2& selectedPosition) const;

private:
    float minimumWeight;
};