#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include "GameObject.h"
#include "Components.h"
#include "GameComponents.h"

// Factory helpers for the objects my game needs.
// Each one just attaches a mix of components.

// Static platform: has a position/size and a tile style, never moves.
inline GameObject makeStaticPlatform(float x, float y, float w, float h,
                                     SDL_Texture* left, SDL_Texture* middle, SDL_Texture* right,
                                     Uint8 r = 255, Uint8 g = 255, Uint8 b = 255)
{
    GameObject obj("static_platform");
    obj.addComponent<TransformComponent>(x, y, w, h);
    obj.addComponent<CollidableComponent>();
    auto& style = obj.addComponent<TileStyleComponent>();
    style.left = left;
    style.middle = middle;
    style.right = right;
    style.tintR = r;
    style.tintG = g;
    style.tintB = b;
    return obj;
}

// Moving platform: like a static one, plus a movement pattern.
// Nothing is sent over the network (Deterministic) because every
// client computes the same position from the shared clock.
inline GameObject makeMovingPlatform(float x, float y, float w, float h,
                                     MoveAxis axis, float minValue, float maxValue, float speed,
                                     SDL_Texture* left, SDL_Texture* right)
{
    GameObject obj = makeStaticPlatform(x, y, w, h, left, nullptr, right);
    auto& pattern = obj.addComponent<MovingPatternComponent>();
    pattern.axis = axis;
    pattern.minValue = minValue;
    pattern.maxValue = maxValue;
    pattern.speed = speed;
    auto& net = obj.addComponent<NetworkComponent>();
    net.networkId = 0;
    net.authority = NetworkAuthority::Deterministic;
    return obj;
}

// Hidden spawn point: just a position and an id.
inline GameObject makeSpawnPoint(int spawnId, float x, float y)
{
    GameObject obj("spawn_point");
    obj.addComponent<TransformComponent>(x, y, 0.0f, 0.0f);
    obj.addComponent<SpawnPointComponent>().spawnId = spawnId;
    return obj;
}

// Hidden death zone: a rectangle that sends the player to a spawn point.
inline GameObject makeDeathZone(float x, float y, float w, float h, int spawnId)
{
    GameObject obj("death_zone");
    obj.addComponent<TransformComponent>(x, y, w, h);
    obj.addComponent<CollidableComponent>().solid = false;
    obj.addComponent<DeathZoneComponent>().spawnId = spawnId;
    return obj;
}

// Hidden scroll boundary: scrolls the screen when the player touches it.
inline GameObject makeScrollTrigger(float x, float y, float w, float h, int direction)
{
    GameObject obj("scroll_trigger");
    obj.addComponent<TransformComponent>(x, y, w, h);
    obj.addComponent<CollidableComponent>().solid = false;
    obj.addComponent<ScrollTriggerComponent>().direction = direction;
    return obj;
}

// Slime enemy: moves on its own and respawns the player on touch.
inline GameObject makeEnemy(float x, float y, float w, float h)
{
    GameObject obj("enemy");
    obj.addComponent<TransformComponent>(x, y, w, h);
    obj.addComponent<CollidableComponent>();
    return obj;
}

#endif