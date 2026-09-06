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

    // Setters and Getters for velocity and gravity
    void setVelocity(float vx, float vy);
    void setVelocityX(float vx);
    void setVelocityY(float vy);

    float getVelocityX() const;
    float getVelocityY() const;

    void setGravityEnabled(bool enabled);
    bool isGravityEnabled() const;

    void setGrounded(bool grounded);
    bool isGrounded() const;

    void setTexture(SDL_Texture* texture);

    void setSpriteSheet(int frameCount, int frameWidth, int frameHeight);
    void updateAnimation();

    // Real-time animation for entities that need an exact frame rate
    void setAnimationSpeed(float framesPerSecond);
    void updateAnimation(float deltaTime);

    // Getters
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

    int frameCount;
    int frameWidth;
    int frameHeight;
    int currentFrame;
    int animationCounter;

    // Only used if setAnimationSpeed() is called
    float frameDuration;
    float animationTimer;
    bool useTimedAnimation;
};

#endif