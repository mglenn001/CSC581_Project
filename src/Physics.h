#ifndef PHYSICS_H
#define PHYSICS_H

#include "Entity.h"

class Physics {
public:
    Physics(float gravity = 900.0f);

    void setGravity(float gravity);
    float getGravity() const;

    void update(Entity& entity, float deltaTime);

    void jump(Entity& entity, float jumpStrength);

private:
    float gravity;
};

#endif