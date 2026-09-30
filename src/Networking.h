#pragma once

#include "SharedData.h"

// Section 5: Runs on its own thread. Sends this client's player directly to
// every other peer and stores the players it hears from in sharedData.
// There is no server in the middle.
void networkingThread(SharedData& sharedData, int clientID);

// Section 5: Where the moving platform is right now.
// Every peer calls this and gets the same answer, because it only depends on
// the shared wall clock (not on this client's timeline or start time).
float getSharedPlatformX();