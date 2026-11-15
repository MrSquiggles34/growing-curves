#include "ImageStippler.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

ImageStippler::ImageStippler(float minimumWeight)
    : minimumWeight(minimumWeight)
{
    if (minimumWeight < 0.0f || minimumWeight > 1.0f)
    {
        throw std::invalid_argument(
            "Minimum stippling weight must be between 0 and 1.");
    }
}

bool ImageStippler::selectPosition(
    const ImageDirectionField& imageField,
    const DistanceMap& distanceMap,
    float minimumDistance,
    std::mt19937& randomGenerator,
    glm::vec2& selectedPosition) const
{
    struct Candidate
    {
        int x;
        int y;
        float cumulativeWeight;
    };

    std::vector<Candidate> candidates;

    float totalWeight = 0.0f;

    int width = imageField.getWidth();
    int height = imageField.getHeight();

    // Find all positions that satisfy the minimum-distance constraint.
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float distance = distanceMap.getDistance(x, y);

            if (distance < minimumDistance)
            {
                continue;
            }

            // Sample the original image edge strength.
            float edgeStrength = imageField.getStrength(
                glm::vec2(
                    static_cast<float>(x),
                    static_cast<float>(y)
                )
            );

            // Stronger image features receive higher selection weights.
            float weight = minimumWeight +
                std::pow(edgeStrength, 2.0f);

            totalWeight += weight;

            candidates.push_back({
                x,
                y,
                totalWeight
                });
        }
    }

    if (candidates.empty() || totalWeight <= 0.0f)
    {
        return false;
    }

    // Select a random value within the total accumulated weight.
    std::uniform_real_distribution<float> distribution(
        0.0f,
        totalWeight
    );

    float selection = distribution(randomGenerator);

    // Find the candidate corresponding to that weighted value.
    for (const Candidate& candidate : candidates)
    {
        if (selection <= candidate.cumulativeWeight)
        {
            selectedPosition = glm::vec2(
                static_cast<float>(candidate.x),
                static_cast<float>(candidate.y)
            );

            return true;
        }
    }

    // Fallback for floating-point rounding.
    const Candidate& lastCandidate = candidates.back();

    selectedPosition = glm::vec2(
        static_cast<float>(lastCandidate.x),
        static_cast<float>(lastCandidate.y)
    );

    return true;
}