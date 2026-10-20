#ifndef GAME_COMPONENTS_H
#define GAME_COMPONENTS_H

#include <SDL3/SDL.h>
#include "Component.h"

// Extra components for my individual game (Milestone 3, Part 2).
// They live here so the team's engine files stay unchanged.
// New kinds of objects = new mixes of components, no new classes.

// Marks an object as a spawn point. It has no RenderableComponent,
// so it is hidden (never drawn).
struct SpawnPointComponent : public Component {
    int spawnId = 0;
};

// Marks a hidden zone that kills the player.
// Touching it sends the player to the spawn point with spawnId.
struct DeathZoneComponent : public Component {
    int spawnId = 0;
};

// Marks a hidden boundary that scrolls the screen when touched.
// -1 = left edge of the screen, +1 = right edge of the screen.
struct ScrollTriggerComponent : public Component {
    int direction = 1;
};

// Tells the renderer how to draw a platform from tiles:
// a left end, a repeating middle, and a right end.
// "tint" gives the platform its own color.
struct TileStyleComponent : public Component {
    SDL_Texture* left = nullptr;
    SDL_Texture* middle = nullptr;
    SDL_Texture* right = nullptr;
    Uint8 tintR = 255;
    Uint8 tintG = 255;
    Uint8 tintB = 255;
};

// Which way a moving platform moves (used for the two patterns).
enum class MoveAxis {
    Horizontal,
    Vertical
};

// Marks a platform that moves back and forth in a pattern.
// Position is computed from the shared wall clock, so every
// client sees it in the same place.
struct MovingPatternComponent : public Component {
    MoveAxis axis = MoveAxis::Horizontal;
    float minValue = 0.0f; // lowest X (or Y) it reaches
    float maxValue = 0.0f; // highest X (or Y) it reaches
    float speed = 1.0f; // how fast the wave runs
};

#endif