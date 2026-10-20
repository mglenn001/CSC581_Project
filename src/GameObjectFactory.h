#ifndef GAME_OBJECT_FACTORY_H
#define GAME_OBJECT_FACTORY_H

#include "GameObject.h"
#include "Components.h"

// Creates a local player object with movement, physics, and rendering components.
inline GameObject makePlayer(int clientId, float x, float y, float w, float h)
{
    GameObject obj("player", clientId);
    obj.addComponent<TransformComponent>(x, y, w, h);
    obj.addComponent<VelocityComponent>();
    obj.addComponent<GravityComponent>();
    obj.addComponent<RenderableComponent>();
    obj.addComponent<CollidableComponent>();
    auto& net = obj.addComponent<NetworkComponent>();
    net.networkId = clientId;
    net.authority = NetworkAuthority::Local;
    return obj;
}

// Creates a deterministic moving platform object.
inline GameObject makePlatform(float x, float y, float w, float h)
{
    GameObject obj("platform", 0);
    obj.addComponent<TransformComponent>(x, y, w, h);
    obj.addComponent<RenderableComponent>();
    obj.addComponent<CollidableComponent>();
    auto& net = obj.addComponent<NetworkComponent>();
    net.networkId = 0;
    net.authority = NetworkAuthority::Deterministic;
    return obj;
}

#endif