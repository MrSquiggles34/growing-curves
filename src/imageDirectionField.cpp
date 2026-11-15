
#include "imageDirectionField.h"

#include "../dependencies/stb_image.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>


ImageDirectionField::ImageDirectionField()
    : width(0),
    height(0)
{
}

int ImageDirectionField::getIndex(int x, int y) const
{
    return y * width + x;
}

bool ImageDirectionField::loadImage(const std::string& filepath)
{
    int channels = 0;

    // Request a single grayscale channel.
    unsigned char* image = stbi_load(
        filepath.c_str(),
        &width,
        &height,
        &channels,
        1
    );

    if (image == nullptr)
    {
        std::cout << "Failed to load image: "
            << stbi_failure_reason() << std::endl;

        width = 0;
        height = 0;
        return false;
    }

    grayscale.resize(width * height);

    for (int i = 0; i < width * height; ++i)
    {
        grayscale[i] = image[i] / 255.0f;
    }

    stbi_image_free(image);

    calculateGradients();

    std::cout << "Image direction field loaded: "
        << width << " x " << height << std::endl;

    return true;
}

void ImageDirectionField::calculateGradients()
{
    directions.resize(width * height);
    magnitudes.resize(width * height);

    float maximumMagnitude = 0.0f;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int left = std::max(x - 1, 0);
            int right = std::min(x + 1, width - 1);
            int top = std::max(y - 1, 0);
            int bottom = std::min(y + 1, height - 1);

            // Sample the 3x3 neighborhood.
            float p00 = grayscale[getIndex(left, top)];
            float p10 = grayscale[getIndex(x, top)];
            float p20 = grayscale[getIndex(right, top)];

            float p01 = grayscale[getIndex(left, y)];
            float p21 = grayscale[getIndex(right, y)];

            float p02 = grayscale[getIndex(left, bottom)];
            float p12 = grayscale[getIndex(x, bottom)];
            float p22 = grayscale[getIndex(right, bottom)];

            // Sobel horizontal gradient.
            float gx =
                -p00 + p20
                - 2.0f * p01 + 2.0f * p21
                - p02 + p22;

            // Sobel vertical gradient.
            float gy =
                -p00 - 2.0f * p10 - p20
                + p02 + 2.0f * p12 + p22;

            float magnitude = std::sqrt(gx * gx + gy * gy);

            // The gradient is perpendicular to the edge.
            // Rotate it 90 degrees to obtain the edge tangent.
            glm::vec2 tangent(-gy, gx);

            if (magnitude > 0.0001f)
            {
                tangent /= magnitude;
            }
            else
            {
                tangent = glm::vec2(0.0f);
            }

            int index = getIndex(x, y);

            directions[index] = tangent;
            magnitudes[index] = magnitude;

            maximumMagnitude = std::max(
                maximumMagnitude,
                magnitude
            );
        }
    }

    // Normalize edge strengths to the range [0, 1].
    if (maximumMagnitude > 0.0001f)
    {
        for (float& magnitude : magnitudes)
        {
            magnitude /= maximumMagnitude;
        }
    }

    smoothDirections(4, 2);
}

glm::vec2 ImageDirectionField::getDirection(
    const glm::vec2& position,
    const glm::vec2& currentDirection
) const
{
    if (width <= 0 || height <= 0)
        return currentDirection;

    int x = static_cast<int>(std::round(position.x));
    int y = static_cast<int>(std::round(position.y));

    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);

    glm::vec2 tangent = smoothedDirections[getIndex(x, y)];

    if (glm::length(tangent) < 0.0001f)
        return currentDirection;

    // Resolve the tangent's directional ambiguity.
    if (glm::dot(tangent, currentDirection) <= 0.0f)
    {
        tangent = -tangent;
    }

    return tangent;
}

float ImageDirectionField::getStrength(
    const glm::vec2& position
) const
{
    if (width <= 0 || height <= 0)
        return 0.0f;

    int x = static_cast<int>(std::round(position.x));
    int y = static_cast<int>(std::round(position.y));

    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);

    return magnitudes[getIndex(x, y)];
}

int ImageDirectionField::getWidth() const
{
    return width;
}

int ImageDirectionField::getHeight() const
{
    return height;
}


void ImageDirectionField::printStrongestEdge() const
{
    float strongestMagnitude = 0.0f;
    int strongestX = 0;
    int strongestY = 0;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float magnitude = magnitudes[getIndex(x, y)];

            if (magnitude > strongestMagnitude)
            {
                strongestMagnitude = magnitude;
                strongestX = x;
                strongestY = y;
            }
        }
    }

    glm::vec2 direction = directions[
        getIndex(strongestX, strongestY)
    ];

    std::cout << "\n--- Strongest Image Edge ---" << std::endl;
    std::cout << "Position: ("
        << strongestX << ", " << strongestY << ")"
        << std::endl;

    std::cout << "Strength: "
        << strongestMagnitude << std::endl;

    std::cout << "Tangent: ("
        << direction.x << ", " << direction.y << ")"
        << std::endl;
}

glm::vec2 ImageDirectionField::blendDirection(
    const glm::vec2& simulationDirection,
    const glm::vec2& position,
    float imageInfluence
) const
{
    glm::vec2 imageDirection = getDirection(
        position,
        simulationDirection
    );

    float edgeStrength = getStrength(position);

    // Boost weaker edge strengths.
    edgeStrength = std::sqrt(edgeStrength);

    float influence = imageInfluence * edgeStrength;

    glm::vec2 combinedDirection =
        simulationDirection * (1.0f - influence) +
        imageDirection * influence;

    float magnitude = glm::length(combinedDirection);

    if (magnitude > 0.0001f)
    {
        return combinedDirection / magnitude;
    }

    return simulationDirection;
}

void ImageDirectionField::smoothDirections(
    int radius,
    int iterations)
{
    if (width <= 0 || height <= 0)
        return;

    smoothedDirections = directions;

    const float spatialSigma = 2.0f;
    const float orientationSigma = 0.3f;

    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        std::vector<glm::vec2> nextDirections(
            width * height,
            glm::vec2(0.0f)
        );

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int centerIndex = getIndex(x, y);

                glm::vec2 centerDirection =
                    smoothedDirections[centerIndex];

                // Find a reference direction.
                // If the center has no valid tangent, use the
                // strongest edge in its neighborhood.
                if (glm::length(centerDirection) < 0.0001f)
                {
                    float strongestMagnitude = 0.0f;

                    for (int offsetY = -radius;
                        offsetY <= radius; ++offsetY)
                    {
                        for (int offsetX = -radius;
                            offsetX <= radius; ++offsetX)
                        {
                            int sampleX = x + offsetX;
                            int sampleY = y + offsetY;

                            if (sampleX < 0 || sampleX >= width ||
                                sampleY < 0 || sampleY >= height)
                            {
                                continue;
                            }

                            int sampleIndex = getIndex(
                                sampleX,
                                sampleY
                            );

                            if (magnitudes[sampleIndex] >
                                strongestMagnitude)
                            {
                                strongestMagnitude =
                                    magnitudes[sampleIndex];

                                centerDirection =
                                    smoothedDirections[sampleIndex];
                            }
                        }
                    }
                }

                if (glm::length(centerDirection) < 0.0001f)
                {
                    nextDirections[centerIndex] =
                        glm::vec2(0.0f);

                    continue;
                }

                centerDirection = glm::normalize(centerDirection);

                glm::vec2 accumulatedDirection(0.0f);
                float totalWeight = 0.0f;

                for (int offsetY = -radius;
                    offsetY <= radius; ++offsetY)
                {
                    for (int offsetX = -radius;
                        offsetX <= radius; ++offsetX)
                    {
                        int sampleX = x + offsetX;
                        int sampleY = y + offsetY;

                        if (sampleX < 0 || sampleX >= width ||
                            sampleY < 0 || sampleY >= height)
                        {
                            continue;
                        }

                        int sampleIndex = getIndex(
                            sampleX,
                            sampleY
                        );

                        glm::vec2 sampleDirection =
                            smoothedDirections[sampleIndex];

                        float sampleLength =
                            glm::length(sampleDirection);

                        if (sampleLength < 0.0001f)
                            continue;

                        sampleDirection /= sampleLength;

                        // Tangents are unoriented. Align neighboring
                        // vectors before accumulating them.
                        float alignment = glm::dot(
                            centerDirection,
                            sampleDirection
                        );

                        if (alignment < 0.0f)
                        {
                            sampleDirection = -sampleDirection;
                            alignment = -alignment;
                        }

                        float distanceSquared =
                            static_cast<float>(
                                offsetX * offsetX +
                                offsetY * offsetY
                                );

                        float spatialWeight = std::exp(
                            -distanceSquared /
                            (2.0f * spatialSigma * spatialSigma)
                        );

                        float orientationDifference =
                            1.0f - alignment;

                        float orientationWeight = std::exp(
                            -(orientationDifference *
                                orientationDifference) /
                            (2.0f * orientationSigma *
                                orientationSigma)
                        );

                        float weight =
                            spatialWeight * orientationWeight;

                        accumulatedDirection +=
                            sampleDirection * weight;

                        totalWeight += weight;
                    }
                }

                if (totalWeight > 0.0001f &&
                    glm::length(accumulatedDirection) > 0.0001f)
                {
                    nextDirections[centerIndex] =
                        glm::normalize(accumulatedDirection);
                }
                else
                {
                    nextDirections[centerIndex] =
                        centerDirection;
                }
            }
        }

        smoothedDirections = nextDirections;
    }
}