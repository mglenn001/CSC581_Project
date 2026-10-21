#pragma once

#include "SharedData.h"

// Handles peer-to-peer networking on its own thread, sending local 
// player state and storing received player data.
void networkingThread(SharedData& sharedData, int clientID);

// Computes a repeating wave value based on the shared wall clock.
// Returns a value between minValue and maxValue.
float getSharedWave(float minValue, float maxValue, double speed);

// Calculates horizontal X position for a moving platform.
float getSharedPlatformX();

// Calculates vertical Y position for a moving platform.
float getSharedPlatformY(float topY, float bottomY);