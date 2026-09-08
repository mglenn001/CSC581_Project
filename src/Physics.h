#ifndef PHYSICS_H
#define PHYSICS_H

#include "Entity.h"

class Physics {
public:
    // Create a physics system with a default gravity value
    Physics(float gravity = 900.0f);

    // Change and get the gravity value
    void setGravity(float gravity);
    float getGravity() const;

    // Update the entity's movement using gravity and velocity
    void update(Entity& entity, float deltaTime);

    // Make the entity jump if it is on the ground
    void jump(Entity& entity, float jumpStrength);

private:
    float gravity;
};

#endif