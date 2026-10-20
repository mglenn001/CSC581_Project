#include "Networking.h"

#include <zmq.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// Section 5: Peer-to-peer settings.
// Client 1 publishes on port 6001, Client 2 on 6002, and so on up to 6004.
#define PEER_BASE_PORT 6000
#define MAX_PEERS 4
// Forget a peer if we haven't heard from it for this long (milliseconds)
#define PEER_TIMEOUT_MS 2000

// Current time in milliseconds on a clock that only moves forward
static int64_t nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

float getSharedWave(float minValue, float maxValue, double speed)
{
    // Seconds since 1970 from the wall clock. Every peer reads the same clock,
    // so every peer gets the same position without asking anyone.
    double seconds = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Keep only the position inside one sine wave cycle.
    // This keeps the number small so we don't lose precision.
    const double TWO_PI = 6.283185307179586;
    double phase = std::fmod(seconds * speed, TWO_PI);

    // Swing between minValue and maxValue
    float middle = (minValue + maxValue) / 2.0f;
    float range = (maxValue - minValue) / 2.0f;
    return middle + static_cast<float>(std::sin(phase)) * range;
}

float getSharedPlatformX()
{
    // Oscillate platform between X = 600 and X = 800
    return getSharedWave(600.0f, 800.0f, 1.5);
}

float getSharedPlatformY(float topY, float bottomY)
{
    // Slower wave so the player has time to ride it up and down
    return getSharedWave(topY, bottomY, 0.8);
}

void networkingThread(SharedData& sharedData, int clientID)
{
    try {
        zmq::context_t context(1);

        // Publisher: sends this player's state to every peer that connects to us
        zmq::socket_t publisher(context, zmq::socket_type::pub);
        publisher.set(zmq::sockopt::linger, 0);
        int myPort = PEER_BASE_PORT + clientID;
        publisher.bind("tcp://*:" + std::to_string(myPort));

        // Subscriber: receives the state of every other peer
        zmq::socket_t subscriber(context, zmq::socket_type::sub);
        subscriber.set(zmq::sockopt::linger, 0);
        subscriber.set(zmq::sockopt::subscribe, "");

        // Connect to every other peer. Peers can start in any order because
        // ZeroMQ keeps retrying until the other peer's port opens.
        for (int peerID = 1; peerID <= MAX_PEERS; ++peerID) {
            if (peerID == clientID) {
                continue; // don't connect to ourselves
            }
            subscriber.connect("tcp://localhost:" + std::to_string(PEER_BASE_PORT + peerID));
        }

        std::cout << "Peer " << clientID << " listening for peers on port " << myPort << std::endl;

        while (sharedData.running.load()) {
            // Copy our player's latest state
            PlayerState mine;
            {
                std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                mine.x = sharedData.playerX;
                mine.y = sharedData.playerY;
                mine.facingRight = sharedData.playerFacingRight;
                mine.row = sharedData.playerRow;
                mine.frame = sharedData.playerFrame;
            }

            // Send our state straight to the other peers.
            // encode() builds TEXT or BINARY depending on the chosen format.
            std::string message = NetFormat::encode(sharedData.networkFormat, clientID, mine);
            publisher.send(zmq::buffer(message), zmq::send_flags::none);

            // Read every message that has arrived. dontwait means we never get
            // stuck waiting on a slow or missing peer.
            while (true) {
                zmq::message_t incoming;
                auto result = subscriber.recv(incoming, zmq::recv_flags::dontwait);
                if (!result) {
                    break; // nothing more waiting
                }

                // decode() detects TEXT or BINARY from the message itself
                int id = 0;
                PlayerState other;
                if (NetFormat::decode(incoming.data(), incoming.size(), id, other)) {
                    if (id != clientID && id >= 1 && id <= MAX_PEERS) {
                        std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                        sharedData.remotePlayers[id] = { other.x, other.y, other.facingRight,
                                                         other.row, other.frame, nowMs() };
                    }
                }
            }

            // Drop peers that went quiet (closed their window) and update connected
            {
                std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                int64_t now = nowMs();
                for (auto it = sharedData.remotePlayers.begin(); it != sharedData.remotePlayers.end(); ) {
                    if (now - it->second.lastHeardMs > PEER_TIMEOUT_MS) {
                        it = sharedData.remotePlayers.erase(it);
                    } else {
                        ++it;
                    }
                }
                sharedData.connected = !sharedData.remotePlayers.empty();
            }

            // Section 4: Sync sleep rate to timeline scale.
            // 0.5x -> ~32 ms, 1.0x -> ~16 ms, 2.0x -> ~8 ms between messages.
            // Each client has its own thread, so one client changing speed
            // does not change how fast the others send.
            int sleepMs = static_cast<int>(16.0 / sharedData.currentTimeScale.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, sleepMs)));
        }

        publisher.close();
        subscriber.close();
        context.close();
    }
    catch (const zmq::error_t& e) {
        if (sharedData.running.load()) {
            std::cerr << "Networking error for client " << clientID << ": " << e.what() << std::endl;
        }
    }
}