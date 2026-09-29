#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>

// What we need to know to draw another player's warrior
struct RemotePlayerState {
    float x;
    float y;
    bool facingRight;
    int row;    // which animation row (idle, walk, run, ...)
    int frame;  // which frame in that row
};

// Data shared between the game loop and the networking thread
struct SharedData {
    // Set to false to tell the networking thread to stop
    std::atomic<bool> running{true};

    // True while the server is answering us
    std::atomic<bool> connected{false};

    // This client's own player (written by the game loop)
    float playerX = 0.0f;
    float playerY = 0.0f;
    bool playerFacingRight = true;
    int playerRow = 0;
    int playerFrame = 0;

    // The other players, as last reported by the server
    std::unordered_map<int, RemotePlayerState> remotePlayers;

    // Protects everything above that is not atomic
    std::mutex playerMutex;
};