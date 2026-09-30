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
    int64_t lastHeardMs; // when we last got a message from this peer (steady clock, ms)
};

// Data shared between the game loop and the networking thread
struct SharedData {
    // Set to false to tell the networking thread to stop
    std::atomic<bool> running{true};
    // True while we are hearing from at least one other peer
    std::atomic<bool> connected{false};
 
    // Section 4: Current scale of game timeline (defaults to 1.0)
    // The networking thread uses it to set how fast it sends messages
    std::atomic<float> currentTimeScale{1.0f};
 
    // This client's own player (written by the game loop)
    float playerX = 0.0f;
    float playerY = 0.0f;
    bool playerFacingRight = true;
    int playerRow = 0;
    int playerFrame = 0;
 
    // Section 5: The other players, as last reported directly by each peer
    std::unordered_map<int, RemotePlayerState> remotePlayers;
    // Protects everything above that is not atomic
    std::mutex playerMutex;
};