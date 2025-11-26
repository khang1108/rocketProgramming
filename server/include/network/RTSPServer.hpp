#ifndef RTSP_SERVER_HPP
#define RTSP_SERVER_HPP

#include "Socket.hpp"
#include "ServerWorker.hpp"
#include "utils/Logger.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

class RTSPServer
{
private:
    int port;
    std::unique_ptr<Socket> listenSocket;
    std::atomic<bool> running{false};

    struct ClientSession {
        int clientId;
        std::unique_ptr<Socket> clientSocket;
        std::unique_ptr<ServerWorker> worker;
        std::thread workerThread;
    };

    std::vector<ClientSession> activeSession;
    std::mutex sessionMutex;
    int nextClientId = 1;
public:
    //@brief: Initialize the TCP listen SOCKET
    RTSPServer(int serverPort);
    /*
     * Run - Main server loop
     * 
     * Algorithm:
     * 1. Bind socket to port
     * 2. Listen for connections
     * 3. Accept clients in loop
     * 4. Create ServerWorker for each client
     * 5. Continue until stopped
     */

    void run();
    void stop();
};
#endif