/*
    Headless game server (no SDL window).
    Multithreaded to handle multiple client connections concurrently.
*/
#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

// The server listens on this port
#define SERVER_PORT 5555
// Forget a client if we haven't heard from it for this long (milliseconds)
#define CLIENT_TIMEOUT_MS 2000

// One connected player
struct PlayerInfo {
    float x;
    float y;
    int facingRight;
    int row;
    int frame;
    std::chrono::steady_clock::time_point lastSeen;
};

// Global state shared across client worker threads
static std::map<int, PlayerInfo> g_players;
static std::mutex g_playersMutex; // Mutex to ensure thread-safe updates to g_players

// Worker function executed concurrently on a dedicated thread for each request
void handleClientRequest(zmq::context_t& context, zmq::message_t requestMsg)
{
    // Parse incoming client request string: "id x y facingRight row frame"
    std::string requestString(static_cast<char*>(requestMsg.data()), requestMsg.size());
    std::istringstream input(requestString);

    int id;
    PlayerInfo info;

    if (input >> id >> info.x >> info.y >> info.facingRight >> info.row >> info.frame) {
        info.lastSeen = std::chrono::steady_clock::now();

        // Safely access shared player registry using a mutex lock
        std::lock_guard<std::mutex> lock(g_playersMutex);
        if (g_players.find(id) == g_players.end()) {
            std::cout << "[Server Thread] Client " << id << " connected." << std::endl;
        }
        g_players[id] = info;
    }
}

int main()
{
    zmq::context_t context(1);

    // ROUTER socket allows handling messages from multiple clients concurrently
    zmq::socket_t serverSocket(context, zmq::socket_type::router);
    serverSocket.bind("tcp://*:" + std::to_string(SERVER_PORT));

    std::cout << "Multithreaded Game Server listening on port " << SERVER_PORT << "..." << std::endl;

    while (true) {
        zmq::message_t clientAddress;
        zmq::message_t emptyDelimiter;
        zmq::message_t requestPayload;

        // Receive multi-part frame safely and evaluate every return value
    auto res1 = serverSocket.recv(clientAddress, zmq::recv_flags::none);
    if (!res1) continue;

    auto res2 = serverSocket.recv(emptyDelimiter, zmq::recv_flags::none);
    if (!res2) continue;

    auto res3 = serverSocket.recv(requestPayload, zmq::recv_flags::none);
    if (!res3) continue;

        // Multithread network server to handle request on a new thread
        std::thread worker(handleClientRequest, std::ref(context), std::move(requestPayload));
        worker.detach(); // Allow worker thread to run independently

        // Clean up inactive clients that timed out / left
        {
            std::lock_guard<std::mutex> lock(g_playersMutex);
            auto now = std::chrono::steady_clock::now();
            for (auto it = g_players.begin(); it != g_players.end(); ) {
                auto silentMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastSeen).count();
                if (silentMs > CLIENT_TIMEOUT_MS) {
                    std::cout << "[Server] Client " << it->first << " left." << std::endl;
                    it = g_players.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Build snapshot response containing state of all connected players
        std::ostringstream output;
        {
            std::lock_guard<std::mutex> lock(g_playersMutex);
            for (const auto& entry : g_players) {
                const PlayerInfo& p = entry.second;
                output << entry.first << " " << p.x << " " << p.y << " "
                       << p.facingRight << " " << p.row << " " << p.frame << ";";
            }
        }

        // Send multi-part response back through ROUTER socket
        serverSocket.send(clientAddress, zmq::send_flags::sndmore);
        serverSocket.send(emptyDelimiter, zmq::send_flags::sndmore);
        std::string replyStr = output.str();
        serverSocket.send(zmq::buffer(replyStr), zmq::send_flags::none);
    }

    return 0;
}