#include "Entity.h"

Entity::Entity(float x, float y, float width, float height)
{
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;

    velocityX = 0.0f;
    velocityY = 0.0f;
    gravityEnabled = false;
    grounded = false;

    texture = nullptr;

    frameCount = 1;
    frameWidth = 0;
    frameHeight = 0;
    currentFrame = 0;
    animationCounter = 0;

    // Timed-animation fields, unused unless setAnimationSpeed() is called
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

    // PIXEL mode (default) leaves everything as-is.
    // PROPORTIONAL mode resizes/repositions relative to the window's current size vs. the reference resolution.
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

    SDL_FRect destinationRect = {
        drawX,
        drawY,
        drawWidth,
        drawHeight
    };

    if (texture != nullptr) {

        // Animated sprite sheet
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

        // Normal static texture
        else {
            SDL_RenderTexture(
                renderer,
                texture,
                nullptr,
                &destinationRect
            );
        }
    }

    // No texture: draw a white rectangle
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
    // Lets a specific entity opt into exact-fps animation instead of the default fixed-tick counter
    if (framesPerSecond <= 0.0f) {
        useTimedAnimation = false;
        return;
    }
    frameDuration = 1.0f / framesPerSecond;
    useTimedAnimation = true;
}

void Entity::updateAnimation(float deltaTime)
{
    if (frameCount <= 1)
    {
        return;
    }
    
    if (!useTimedAnimation) {
        updateAnimation();
        return;
    }

    animationTimer += deltaTime;

    if (animationTimer >= frameDuration) {
        currentFrame = (currentFrame + 1) % frameCount;
        animationTimer -= frameDuration; // keep leftover time instead of dropping it
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