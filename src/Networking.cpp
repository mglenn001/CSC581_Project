#include "Networking.h"

#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// The server listens on this port
#define SERVER_ADDRESS "tcp://localhost:5555"

// How long to wait for a reply before trying again (milliseconds)
#define REPLY_TIMEOUT_MS 500

// Make a fresh REQ socket connected to the server
static zmq::socket_t makeSocket(zmq::context_t& context)
{
    zmq::socket_t socket(context, zmq::socket_type::req);
    socket.set(zmq::sockopt::linger, 0); // don't hang on exit
    socket.set(zmq::sockopt::rcvtimeo, REPLY_TIMEOUT_MS); // don't wait forever
    socket.connect(SERVER_ADDRESS);
    return socket;
}

void networkingThread(SharedData& sharedData, int clientID)
{
    try {
        zmq::context_t context(1);
        zmq::socket_t requester = makeSocket(context);

        std::cout << "Client " << clientID << " networking thread started." << std::endl;

        while (sharedData.running.load()) {
            // Copy our player's latest state
            float x, y;
            bool facingRight;
            int row, frame;
            {
                std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                x = sharedData.playerX;
                y = sharedData.playerY;
                facingRight = sharedData.playerFacingRight;
                row = sharedData.playerRow;
                frame = sharedData.playerFrame;
            }

            // Message format: id x y facingRight row frame
            std::ostringstream request;
            request << clientID << " " << x << " " << y << " "
                    << (facingRight ? 1 : 0) << " " << row << " " << frame;
            requester.send(zmq::buffer(request.str()), zmq::send_flags::none);

            // Wait for the server's reply
            zmq::message_t reply;
            auto result = requester.recv(reply, zmq::recv_flags::none);

            if (!result) {
                // No reply (server is down or slow). A REQ socket can't send
                // again until it gets a reply, so throw it away and make a new one.
                sharedData.connected = false;
                requester.close();
                requester = makeSocket(context);
                continue;
            }
            sharedData.connected = true;

            // Reply format: id x y facingRight row frame;id x y facingRight row frame;...
            std::string replyString(static_cast<char*>(reply.data()), reply.size());
            std::stringstream entries(replyString);
            std::string entry;

            std::unordered_map<int, RemotePlayerState> players;

            while (std::getline(entries, entry, ';')) {
                std::stringstream fields(entry);
                int id, facing, remoteRow, remoteFrame;
                float remoteX, remoteY;

                if (fields >> id >> remoteX >> remoteY >> facing >> remoteRow >> remoteFrame) {
                    // Skip ourselves, the game already draws our own player
                    if (id != clientID) {
                        players[id] = { remoteX, remoteY, facing != 0, remoteRow, remoteFrame };
                    }
                }
            }

            // Swap in the new list of other players
            {
                std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                sharedData.remotePlayers = players;
            }

            // About 60 updates per second
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        requester.close();
        context.close();
    }
    catch (const zmq::error_t& e) {
        if (sharedData.running.load()) {
            std::cerr << "Networking error for client " << clientID << ": " << e.what() << std::endl;
        }
    }
}