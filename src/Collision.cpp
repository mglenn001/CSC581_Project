#include "Collision.h"
#include <SDL3/SDL.h>

bool Collision::checkCollision(const Entity& entityA, const Entity& entityB) {
    // Create rectangular bounds for each entity
    SDL_FRect rectA = { entityA.getX(), entityA.getY(), entityA.getWidth(), entityA.getHeight() };
    SDL_FRect rectB = { entityB.getX(), entityB.getY(), entityB.getWidth(), entityB.getHeight() };

    // SDL3's built-in AABB intersection test (bounding box collision, as recommended in the milestone)
    return SDL_HasRectIntersectionFloat(&rectA, &rectB);
}