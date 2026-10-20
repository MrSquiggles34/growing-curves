
#include "Particle.h"

#include <stdexcept>

Particle::Particle(glm::vec2 position, glm::vec2 velocity, float mass)
    : position(position), velocity(velocity), mass(mass)
{
    if (mass <= 0.0f)
    {
        throw std::invalid_argument("Particle mass must be positive.");
    }
}

void Particle::update(glm::vec2 force, float deltaTime)
{
    glm::vec2 acceleration = force / mass;

    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;
}

glm::vec2 Particle::getPosition() const
{
    return position;
}

glm::vec2 Particle::getVelocity() const
{
    return velocity;
}
