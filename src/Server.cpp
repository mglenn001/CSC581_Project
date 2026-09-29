/*
    Headless game server (no SDL window).
    Every client sends its own player's state and gets back everyone's state,
    so movement on one client shows up on the others.
*/

#include <zmq.hpp>

#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

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

int main()
{
    zmq::context_t context(1);

    // REP socket: answers one request at a time, so no mutex is needed
    zmq::socket_t responder(context, zmq::socket_type::rep);
    responder.bind("tcp://*:" + std::to_string(SERVER_PORT));

    std::cout << "Game server listening on port " << SERVER_PORT << "..." << std::endl;

    // Latest state of every client, by client ID
    std::map<int, PlayerInfo> players;

    while (true) {
        zmq::message_t request;
        auto result = responder.recv(request, zmq::recv_flags::none);
        if (!result) {
            continue;
        }

        // Request format: id x y facingRight row frame
        std::string requestString(static_cast<char*>(request.data()), request.size());
        std::istringstream input(requestString);

        int id;
        PlayerInfo info;

        if (input >> id >> info.x >> info.y >> info.facingRight >> info.row >> info.frame) {
            info.lastSeen = std::chrono::steady_clock::now();

            if (players.find(id) == players.end()) {
                std::cout << "Client " << id << " joined." << std::endl;
            }
            players[id] = info;
        }

        // Drop clients that stopped talking to us
        auto now = std::chrono::steady_clock::now();
        for (auto it = players.begin(); it != players.end(); ) {
            auto silentMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastSeen).count();
            if (silentMs > CLIENT_TIMEOUT_MS) {
                std::cout << "Client " << it->first << " left." << std::endl;
                it = players.erase(it);
            } else {
                ++it;
            }
        }

        // Reply with every player: id x y facingRight row frame;
        std::ostringstream output;
        for (const auto& entry : players) {
            const PlayerInfo& p = entry.second;
            output << entry.first << " " << p.x << " " << p.y << " "
                   << p.facingRight << " " << p.row << " " << p.frame << ";";
        }

        // A REP socket must always reply, even with an empty string
        responder.send(zmq::buffer(output.str()), zmq::send_flags::none);
    }

    return 0;
}