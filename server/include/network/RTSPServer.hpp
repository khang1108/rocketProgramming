#ifndef SERVER_NETWORK_RTSPSERVER_HPP
#define SERVER_NETWORK_RTSPSERVER_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "ServerWorker.hpp"
#include "Socket.hpp"
#include "utils/Logger.hpp"

/**
 * @class RTSPServer
 * @brief RTSP server implementation for handling multiple client connections
 *
 * @details
 * RTSPServer manages:
 * - TCP socket listening on RTSP port (default 8554)
 * - Accepting incoming client connections
 * - Creating ServerWorker threads for each client
 * - Managing active client sessions
 * - Clean shutdown of all connections
 */
class RTSPServer {
  private:
    int port;
    std::unique_ptr<Socket> listenSocket;
    std::atomic<bool> running{false};

    /**
     * @struct ClientSession
     * @brief Represents an active client connection and its associated resources
     */
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