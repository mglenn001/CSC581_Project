#include "Collision.h"
#include <SDL3/SDL.h>

bool Collision::checkCollision(const Entity& entityA, const Entity& entityB) {
    // Create a rectangle around each entity
    SDL_FRect rectA = { entityA.getX(), entityA.getY(), entityA.getWidth(), entityA.getHeight() };
    SDL_FRect rectB = { entityB.getX(), entityB.getY(), entityB.getWidth(), entityB.getHeight() };

    // Check if the two rectangles overlap
    return SDL_HasRectIntersectionFloat(&rectA, &rectB);
}