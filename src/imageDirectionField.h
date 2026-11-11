
#pragma once

#include <glm/vec2.hpp>

#include <string>
#include <vector>

class ImageDirectionField
{
public:
    ImageDirectionField();

    bool loadImage(const std::string& filepath);

    glm::vec2 getDirection(
        const glm::vec2& position,
        const glm::vec2& currentDirection
    ) const;

    glm::vec2 blendDirection(
        const glm::vec2& simulationDirection,
        const glm::vec2& position,
        float imageInfluence
    ) const;

    float getStrength(const glm::vec2& position) const;

    int getWidth() const;
    int getHeight() const;

    void printStrongestEdge() const;



private:
    int width;
    int height;

    std::vector<float> grayscale;
    std::vector<glm::vec2> directions;
    std::vector<float> magnitudes;

    int getIndex(int x, int y) const;

    void calculateGradients();

};
