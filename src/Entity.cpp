#include "Entity.h"

Entity::Entity(float x, float y, float width, float height)
{
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;

    // Start the entity with no movement
    velocityX = 0.0f;
    velocityY = 0.0f;

    // Gravity is off by default
    gravityEnabled = false;
    grounded = false;

    // No texture is assigned when the entity is created
    texture = nullptr;

    // Default animation values
    frameCount = 1;
    frameWidth = 0;
    frameHeight = 0;
    currentFrame = 0;
    animationCounter = 0;

    // These values are only used for timed animation
    frameDuration = 0.0f;
    animationTimer = 0.0f;
    useTimedAnimation = false;
}

void Entity::render(SDL_Renderer* renderer)
{
    float drawX = x;
    float drawY = y;
    float drawWidth = width;
    float drawHeight = height;

    // In PIXEL mode, use the entity's original position and size
    // In PROPORTIONAL mode, scale the entity to match the window size
    if (Scaling::getMode() == ScalingMode::PROPORTIONAL) {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetRenderOutputSize(renderer, &windowWidth, &windowHeight);

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        Scaling::getScaleFactors(windowWidth, windowHeight, scaleX, scaleY);

        drawX = x * scaleX;
        drawY = y * scaleY;
        drawWidth = width * scaleX;
        drawHeight = height * scaleY;
    }

    // Create the rectangle where the entity will be drawn
    SDL_FRect destinationRect = {
        drawX,
        drawY,
        drawWidth,
        drawHeight
    };

    if (texture != nullptr) {

        // Use part of the sprite sheet when animation is enabled
        if (frameCount > 1 && frameWidth > 0 && frameHeight > 0) {

            SDL_FRect sourceRect = {
                (float)(currentFrame * frameWidth),
                0.0f,
                (float)frameWidth,
                (float)frameHeight
            };

            SDL_RenderTexture(
                renderer,
                texture,
                &sourceRect,
                &destinationRect
            );
        }
        // Draw the full texture when the entity is not animated
        else {
            SDL_RenderTexture(
                renderer,
                texture,
                nullptr,
                &destinationRect
            );
        }
    }
    // Draw a white rectangle if no texture has been assigned
    else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &destinationRect);
    }
}

void Entity::setPosition(float x, float y)
{
    this->x = x;
    this->y = y;
}

void Entity::move(float dx, float dy)
{
    x += dx;
    y += dy;
}

void Entity::setTexture(SDL_Texture* texture)
{
    this->texture = texture;
}

void Entity::setSpriteSheet(int frameCount, int frameWidth, int frameHeight)
{
    this->frameCount = frameCount;
    this->frameWidth = frameWidth;
    this->frameHeight = frameHeight;
}

void Entity::updateAnimation()
{
    // Static images do not need animation
    if (frameCount <= 1) {
        return;
    }

    animationCounter++;

    if (animationCounter >= 100) {
        currentFrame = (currentFrame + 1) % frameCount;
        animationCounter = 0;
    }
}

void Entity::setAnimationSpeed(float framesPerSecond)
{
    // Turn off timed animation if the frame rate is not valid
    if (framesPerSecond <= 0.0f) {
        useTimedAnimation = false;
        return;
    }
    // Convert frames per second into the time for one frame
    frameDuration = 1.0f / framesPerSecond;
    useTimedAnimation = true;
}

void Entity::updateAnimation(float deltaTime)
{
    // A single-frame image does not need animation
    if (frameCount <= 1)
    {
        return;
    }
    // Use the normal animation method if timed animation is not enabled
    if (!useTimedAnimation) {
        updateAnimation();
        return;
    }

    // Add the time since the last update
    animationTimer += deltaTime;

    // Change to the next frame when enough time has passed
    if (animationTimer >= frameDuration) {
        currentFrame = (currentFrame + 1) % frameCount;
        animationTimer -= frameDuration; // keep any extra time for the next animation update
    }
}

float Entity::getX() const
{
    return x;
}

float Entity::getY() const
{
    return y;
}

float Entity::getWidth() const
{
    return width;
}

float Entity::getHeight() const
{
    return height;
}

void Entity::setVelocity(float vx, float vy)
{
    velocityX = vx;
    velocityY = vy;
}

void Entity::setVelocityX(float vx)
{
    velocityX = vx;
}

void Entity::setVelocityY(float vy)
{
    velocityY = vy;
}

float Entity::getVelocityX() const
{
    return velocityX;
}

float Entity::getVelocityY() const
{
    return velocityY;
}

void Entity::setGravityEnabled(bool enabled)
{
    gravityEnabled = enabled;
}

bool Entity::isGravityEnabled() const
{
    return gravityEnabled;
}

void Entity::setGrounded(bool grounded)
{
    this->grounded = grounded;
}

bool Entity::isGrounded() const
{
    return grounded;
}