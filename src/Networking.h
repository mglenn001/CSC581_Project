#pragma once

#include "SharedData.h"

// Runs on its own thread. Sends this client's player to the server and
// stores the other players it gets back in sharedData.
void networkingThread(SharedData& sharedData, int clientID);