#include "Physics.h"

Physics::Physics(float gravity)
{
    // Store the gravity value used by the physics system
    this->gravity = gravity;
}

void Physics::setGravity(float gravity)
{
    this->gravity = gravity;
}

float Physics::getGravity() const
{
    return gravity;
}

void Physics::update(Entity& entity, float deltaTime)
{
    // Do not apply gravity if it is disabled for this entity
    if (!entity.isGravityEnabled()) {
        return;
    }

    float velocityY = entity.getVelocityY();

    // Increase downward speed based on gravity and time
    velocityY += gravity * deltaTime;

    entity.setVelocityY(velocityY);

    // Move the entity based on its current velocity
    entity.move(
        entity.getVelocityX() * deltaTime,
        velocityY * deltaTime
    );
}

void Physics::jump(Entity& entity, float jumpStrength)
{
    // The entity can only jump when it is on the ground
    if (!entity.isGrounded()) {
        return;
    }

    // A negative Y velocity moves the entity upward in SDL coordinates
    entity.setVelocityY(-jumpStrength);

    // The entity is no longer on the ground after jumping
    entity.setGrounded(false);
}