/*
 * SERVER MAIN - Entry Point
 * 
 * Usage: ./server <port>
 * Example: ./server 8554
 * 
 * Responsibilities:
 * 1. Parse command line arguments
 * 2. Initialize server
 * 3. Start listening for clients
 * 4. Handle graceful shutdown
 */

#include "network/RTSPServer.hpp"
#include "utils/Logger.hpp"
#include "utils/Config.hpp"
#include <csignal>
#include <iostream>
#include <memory>

// Global server instance (for signal handling)
std::unique_ptr<RTSPServer> g_server;

void signalHandler(int signal);
int main(int argc, char *argv[]) 
{
    try {
        if(argc != 2) {
            std::cerr << "Usage: " << argv[0] << "<port>" << std::endl;
            return 1;
        }

        int port = std::stoi(argv[1]);

        if(port < 1024 || port > 65535){
            std::cerr << "Error: You should choose the port between 1024 and 65535" << std::endl;
            return 1;
        }

        // Initialize logger
        Logger::getInstance().initialize("server.log", LogLevel::DEBUG);
        Logger::getInstance().log(INFO, "=== RTSP/RTP Video Streaming Server ===");
        Logger::getInstance().log(INFO, "Port: " + std::to_string(port));
        
        // Load configuration
        Config::getInstance().load("config/server.conf");
        
        // Setup signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Create and start server
        g_server = std::make_unique<RTSPServer>(port);
        
        Logger::getInstance().log(INFO, "Server starting on port " + std::to_string(port));
        Logger::getInstance().log(INFO, "Press Ctrl+C to stop");
        
        // Run server (blocking)
        g_server->run();
        
        Logger::getInstance().log(INFO, "Server stopped");
    } catch (const std::exception& e){
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Logger::getInstance().log(ERROR, "Fatal error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}