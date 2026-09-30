/*
    Headless game server (no SDL window).
    Section 3: Multithreaded to handle multiple client connections concurrently.
    Section 4: Asynchronous server with moving platform authority
*/
#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <cmath>
#include <vector>

// Base port for client connections
#define BASE_PORT 5555 // Client 1 = 5556, Client 2 = 5557, Client 3 = 5558
// Forget a client if we haven't heard from it for this long (milliseconds)
#define CLIENT_TIMEOUT_MS 2000
#define MAX_CLIENTS 3

// One connected player
struct PlayerInfo {
    float x = 0.0f;
    float y = 0.0f;
    int facingRight = 1;
    int row = 0;
    int frame = 0;
    std::chrono::steady_clock::time_point lastSeen;
};

// Global state shared across client worker threads
static std::map<int, PlayerInfo> g_players;
static std::mutex g_playersMutex; // mutex to ensure thread-safe updates to g_players

// Server-authoritative moving platform state
static float g_platformX = 600.0f;
static std::mutex g_platformMutex;

// Dedicated thread worker handling synchronous communications for a client slot
void clientWorkerThread(int clientID, int port) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.set(zmq::sockopt::linger, 0);
    socket.bind("tcp://*:" + std::to_string(port));

    // Locked console print to prevent garbled multithreaded output
    static std::mutex coutMutex;
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[Server] Dedicated thread listening for Client " << clientID 
                  << " on port " << port << std::endl;
    }

    while (true) {
        zmq::message_t requestPayload;
        auto res = socket.recv(requestPayload, zmq::recv_flags::none);
        if (!res) continue;

        // Parse client update
        std::string reqStr(static_cast<char*>(requestPayload.data()), requestPayload.size());
        std::istringstream input(reqStr);
        int id;
        PlayerInfo info;

        if (input >> id >> info.x >> info.y >> info.facingRight >> info.row >> info.frame) {
            info.lastSeen = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(g_playersMutex);
            g_players[id] = info;
        }

        // Clean up timed out clients
        {
            std::lock_guard<std::mutex> lock(g_playersMutex);
            auto now = std::chrono::steady_clock::now();
            for (auto it = g_players.begin(); it != g_players.end(); ) {
                auto silentMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastSeen).count();
                if (silentMs > CLIENT_TIMEOUT_MS) {
                    it = g_players.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Construct snapshot response containing all players + platform position
        std::ostringstream output;
        {
            std::lock_guard<std::mutex> platLock(g_platformMutex);
            output << "PLAT " << g_platformX << ";";
        }

        {
            std::lock_guard<std::mutex> lock(g_playersMutex);
            for (const auto& entry : g_players) {
                const PlayerInfo& p = entry.second;
                output << entry.first << " " << p.x << " " << p.y << " "
                       << p.facingRight << " " << p.row << " " << p.frame << ";";
            }
        }

        std::string replyStr = output.str();
        socket.send(zmq::buffer(replyStr), zmq::send_flags::none);
    }
}

// Background thread updating moving platforms continuously
void platformUpdateThread() {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - start).count();
        
        // Oscillate platform between X = 600 and X = 800
        float newX = 700.0f + std::sin(elapsed * 1.5f) * 100.0f;
        {
            std::lock_guard<std::mutex> lock(g_platformMutex);
            g_platformX = newX;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main() {
    std::cout << "Starting Asynchronous Multithreaded Server..." << std::endl;

    std::thread platThread(platformUpdateThread);
    platThread.detach();

    std::vector<std::thread> workers;
    for (int i = 1; i <= MAX_CLIENTS; ++i) {
        workers.emplace_back(clientWorkerThread, i, BASE_PORT + i);
    }

    for (auto& w : workers) {
        w.join();
    }

    return 0;
}