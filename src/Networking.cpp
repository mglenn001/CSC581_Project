#include "Networking.h"

#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

void networkingThread(SharedData& sharedData, int clientID)
{
    try {
        zmq::context_t context(1);
        // Client 1 connects to 5556, Client 2 connects to 5557, etc.
        std::string serverAddr = "tcp://localhost:" + std::to_string(5555 + clientID);
        
        zmq::socket_t requester(context, zmq::socket_type::req);
        requester.set(zmq::sockopt::linger, 0);
        requester.set(zmq::sockopt::rcvtimeo, 500);
        requester.connect(serverAddr);

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
            request << clientID << " " << x << " " << y << " " << (facingRight ? 1 : 0) << " " << row << " " << frame;
            requester.send(zmq::buffer(request.str()), zmq::send_flags::none);

            // Wait for the server's reply
            zmq::message_t reply;
            auto result = requester.recv(reply, zmq::recv_flags::none);

            if (!result) {
                sharedData.connected = false;
                requester.close();
                requester = zmq::socket_t(context, zmq::socket_type::req);
                requester.connect(serverAddr);
                continue;
            }
            sharedData.connected = true;

            // Reply format: id x y facingRight row frame;id x y facingRight row frame;...
            std::string replyString(static_cast<char*>(reply.data()), reply.size());
            std::stringstream entries(replyString);
            std::string entry;

            std::unordered_map<int, RemotePlayerState> players;

            while (std::getline(entries, entry, ';')) {
                if (entry.rfind("PLAT", 0) == 0) {
                    std::stringstream platStream(entry);
                    std::string label;
                    float platX;
                    if (platStream >> label >> platX) {
                        std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                        sharedData.movingPlatformX = platX;
                    }
                    continue;
                }

                std::stringstream fields(entry);
                int id, facing, remoteRow, remoteFrame;
                float remoteX, remoteY;
                if (fields >> id >> remoteX >> remoteY >> facing >> remoteRow >> remoteFrame) {
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

            // Sync sleep rate to timeline scale for asynchronous demonstration
            int sleepMs = static_cast<int>(16.0 / sharedData.currentTimeScale.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, sleepMs)));
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