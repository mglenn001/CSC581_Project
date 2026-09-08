#ifndef ENTITY_H
#define ENTITY_H

#include <SDL3/SDL.h>
#include "Scaling.h"

class Entity {
public:
    Entity(float x, float y, float width, float height);

    void render(SDL_Renderer* renderer);

    void setPosition(float x, float y);
    void move(float dx, float dy);

    // Set the entity's movement speed
    void setVelocity(float vx, float vy);
    void setVelocityX(float vx);
    void setVelocityY(float vy);

    float getVelocityX() const;
    float getVelocityY() const;

    // Turn gravity on or off for this entity
    void setGravityEnabled(bool enabled);
    bool isGravityEnabled() const;

    // Track whether the entity is touching the ground
    void setGrounded(bool grounded);
    bool isGrounded() const;

    // Set the image used to draw the entity
    void setTexture(SDL_Texture* texture);

    // Set up the sprite sheet animation
    void setSpriteSheet(int frameCount, int frameWidth, int frameHeight);
    void updateAnimation();

    // Set the animation speed when an exact frame rate is needed
    void setAnimationSpeed(float framesPerSecond);
    void updateAnimation(float deltaTime);

    // Get the entity's position and size
    float getX() const;
    float getY() const;
    float getWidth() const;
    float getHeight() const;

private:
    float x;
    float y;
    float width;
    float height;
    float velocityX;
    float velocityY;
    bool gravityEnabled;
    bool grounded;

    SDL_Texture* texture;

    // Sprite sheet information
    int frameCount;
    int frameWidth;
    int frameHeight;
    int currentFrame;
    int animationCounter;

    // Used for animations with a specific frame rate.
    float frameDuration;
    float animationTimer;
    bool useTimedAnimation;
};

#endif