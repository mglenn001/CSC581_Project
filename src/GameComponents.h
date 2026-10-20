#ifndef GAME_COMPONENTS_H
#define GAME_COMPONENTS_H

#include <SDL3/SDL.h>
#include "Component.h"

// Extra components for individual game features (Milestone 3, Part 2).

// Marks a hidden spawn point location.
struct SpawnPointComponent : public Component {
    int spawnId = 0;
};

// Marks a hidden zone that kills the player and resets them to a spawn point.
struct DeathZoneComponent : public Component {
    int spawnId = 0;
};

// Marks a hidden trigger that scrolls the screen (-1 = left, +1 = right).
struct ScrollTriggerComponent : public Component {
    int direction = 1;
};

// Stores tile textures (left, middle, right) and tint color for drawing platforms.
struct TileStyleComponent : public Component {
    SDL_Texture* left = nullptr;
    SDL_Texture* middle = nullptr;
    SDL_Texture* right = nullptr;
    Uint8 tintR = 255;
    Uint8 tintG = 255;
    Uint8 tintB = 255;
};

// Movement direction for moving platforms.
enum class MoveAxis {
    Horizontal,
    Vertical
};

// Marks a platform that moves back and forth in a repeating pattern.
struct MovingPatternComponent : public Component {
    MoveAxis axis = MoveAxis::Horizontal;
    float minValue = 0.0f; // minimum X or Y position
    float maxValue = 0.0f; // maximum X or Y position
    float speed = 1.0f; // movement speed
};

#endif