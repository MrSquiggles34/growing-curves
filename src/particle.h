
#pragma once

#include <glm/vec2.hpp>

class Particle
{
public:
    Particle(glm::vec2 position, glm::vec2 velocity = glm::vec2(0.0f),
        float mass = 1.0f);

    void update(glm::vec2 force, float deltaTime);

    glm::vec2 getPosition() const;
    glm::vec2 getVelocity() const;

private:
    glm::vec2 position;
    glm::vec2 velocity;

    float mass;
};
