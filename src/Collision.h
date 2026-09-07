#ifndef COLLISION_H
#define COLLISION_H

#include "Entity.h"

class Collision {
public:
    // Check if two entities are touching each other
    static bool checkCollision(const Entity& entityA, const Entity& entityB);
};

#endif