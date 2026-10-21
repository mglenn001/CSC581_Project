#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <SDL3/SDL.h>
#include "Component.h"

// TransformComponent: Stores position and size for a game object.
struct TransformComponent : public Component {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    TransformComponent() = default;
    TransformComponent(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height) {}
};

// VelocityComponent: Stores movement speed for moving objects.
struct VelocityComponent : public Component {
    float vx = 0.0f;
    float vy = 0.0f;
};

// GravityComponent: Tells the physics system to apply gravity to this object.
struct GravityComponent : public Component {
    bool grounded = false;
};

// RenderableComponent: Stores texture and animation data for drawing the object.
struct RenderableComponent : public Component {
    SDL_Texture* texture = nullptr;

    int frameCount = 1;
    int frameWidth = 0;
    int frameHeight = 0;
    int currentFrame = 0;
    int animationCounter = 0;

    float frameDuration = 0.0f;
    float animationTimer = 0.0f;
    bool useTimedAnimation = false;
};

// CollidableComponent: Marks an object as something that can collide with others.
struct CollidableComponent : public Component {
    bool solid = true;
};

// NetworkComponent: Tracks how an object's state is synchronized over the network.
enum class NetworkAuthority {
    Local, // owned by this client
    RemoteAuthoritative, // owned by the server
    RemoteMirrored, // owned by another client
    Deterministic // computed independently by every peer
};

struct NetworkComponent : public Component {
    int networkId = -1;
    NetworkAuthority authority = NetworkAuthority::Local;
};

#endif