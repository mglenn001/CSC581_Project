#include "Physics.h"

Physics::Physics(float gravity)
{
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
    if (!entity.isGravityEnabled()) {
        return;
    }

    float velocityY = entity.getVelocityY();

    // Constant downward acceleration
    velocityY += gravity * deltaTime;

    entity.setVelocityY(velocityY);

    // Move entity follow its current velocity
    entity.move(
        entity.getVelocityX() * deltaTime,
        velocityY * deltaTime
    );
}

void Physics::jump(Entity& entity, float jumpStrength)
{
    if (!entity.isGrounded()) {
        return;
    }

    // Negative Y moves upward in SDL coordinates
    entity.setVelocityY(-jumpStrength);
    entity.setGrounded(false);
}