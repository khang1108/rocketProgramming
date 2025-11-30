/**
 * @file client.cpp
 * @brief RTSP Client Application Entry Point
 */

#include <iostream>
#include <string>
#include "ui/ClientUI.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"

/**
 * @brief Client application entry point
 *
 * Usage:
 *   ./client <server_ip> <server_port> <video_file> [rtp_port]
 *
 * Example:
 *   ./client 127.0.0.1 8554 movie.Mjpeg
 *   ./client 127.0.0.1 8554 movie.Mjpeg 25000
 */
int main(int argc, char* argv[]) {
    try {
        // ==================== INITIALIZATION ====================
        
        // Initialize logger
        Logger::getInstance().initialize("client.log", LogLevel::INFO);
        
        LOG_INFO("========================================");
        LOG_INFO("   RTSP/RTP Video Streaming Client");
        LOG_INFO("========================================");
        
        // ==================== PARSE ARGUMENTS ====================
        
        std::string serverIP = "127.0.0.1";
        int serverPort = 8554;
        std::string videoFile = "movie.Mjpeg";
        int clientRTPPort = 25000;
        
        // Parse command line arguments
        if (argc >= 2) {
            serverIP = argv[1];
        }
        
        if (argc >= 3) {
            serverPort = std::stoi(argv[2]);
        }
        
        if (argc >= 4) {
            videoFile = argv[3];
        }
        
        if (argc >= 5) {
            clientRTPPort = std::stoi(argv[4]);
        }
        
        // Display configuration
        std::cout << "Configuration:\n";
        std::cout << "  Server IP:    " << serverIP << "\n";
        std::cout << "  Server Port:  " << serverPort << "\n";
        std::cout << "  Video File:   " << videoFile << "\n";
        std::cout << "  RTP Port:     " << clientRTPPort << "\n";
        std::cout << "========================================\n\n";
        
        LOG_INFO("Server: " + serverIP + ":" + std::to_string(serverPort));
        LOG_INFO("Video: " + videoFile);
        LOG_INFO("RTP Port: " + std::to_string(clientRTPPort));
        
        // ==================== CREATE AND RUN CLIENT UI ====================
        
        // Create ClientUI
        ClientUI clientUI(serverIP, serverPort, videoFile, clientRTPPort);
        
        // Initialize UI components
        if (!clientUI.initialize()) {
            std::cerr << "Failed to initialize ClientUI\n";
            LOG_ERROR("Failed to initialize ClientUI");
            return 1;
        }
        
        LOG_INFO("ClientUI initialized successfully");
        
        // Run UI (blocking until user closes window)
        clientUI.run();
        
        // ==================== CLEANUP ====================
        
        LOG_INFO("Client application exiting");
        std::cout << "\nThank you for using RTSP Client!\n";
        
    } catch (const std::exception& e) {
        LOG_ERROR("FATAL ERROR: " + std::string(e.what()));
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}