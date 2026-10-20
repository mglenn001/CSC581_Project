#pragma once

#include "SharedData.h"

// Runs on its own thread. Sends this client's player directly to
// every other peer and stores the players it hears from in sharedData.
// There is no server in the middle (peer-to-peer).
void networkingThread(SharedData& sharedData, int clientID);

// Shared wave used by the moving platforms.
// Every peer gets the same answer because it only depends on the shared
// wall clock (not on this client's timeline or start time).
// Returns a value that swings between minValue and maxValue.
float getSharedWave(float minValue, float maxValue, double speed);

// Horizontal moving platform: X swings between 600 and 800.
float getSharedPlatformX();

// Vertical moving platform: Y swings between topY and bottomY.
float getSharedPlatformY(float topY, float bottomY);