/**
 * @file main.cpp
 * @brief RTSP Client Entry Point
 *
 * @details
 * Day 1 Skeleton - Developer C
 * Simple main function to test Logger and Config initialization
 * Full RTSP client implementation will be added in Day 2+
 */

#include <iostream>
#include <string>
#include "network/RTSPClient.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"

/**
 * @brief Client application entry point
 *
 * @details
 * Day 1: Basic skeleton - tests utilities only
 * Day 2+: Full RTSP client implementation
 *
 * Usage:
 *   ./client [server_ip] [server_port]
 *
 * Example:
 *   ./client 127.0.0.1 8554
 */
int main(int argc, char* argv[]) {
    try {
        // ==================== INITIALIZATION ====================

        // Initialize logger (removed DEBUG to avoid conflicts)
        Logger::getInstance().initialize("client.log", LogLevel::INFO);

        LOG_INFO("========================================");
        LOG_INFO("   RTP/RTSP Video Streaming Client");
        LOG_INFO("   Day 1 - Skeleton Implementation");
        LOG_INFO("========================================");

        // Initialize config
        Config::initialize("client.conf");
        auto& config = Config::getInstance();

        LOG_INFO("Configuration loaded:");
        config.dump();

        // ==================== COMMAND LINE PARSING ====================

        std::string serverIP = config.getString("server_ip", "127.0.0.1");
        int serverPort = config.getInt("server_port", 8554);

        // Command line overrides config
        if (argc >= 2) {
            serverIP = argv[1];
            LOG_INFO("Server IP (from command line): " + serverIP);
        }

        if (argc >= 3) {
            serverPort = std::stoi(argv[2]);
            LOG_INFO("Server port (from command line): " + std::to_string(serverPort));
        }

        LOG_INFO("Target server: " + serverIP + ":" + std::to_string(serverPort));

        // ==================== DAY 1: TEST UTILITIES ====================

        LOG_INFO("\n=== Day 1 Tests ===");

        // Test 1: Logger levels
        LOG_INFO("This is an INFO message");
        LOG_WARN("This is a WARNING message");
        LOG_ERROR("This is an ERROR message");

        // Test 2: Config reading
        LOG_INFO("\nConfig test:");
        LOG_INFO("  server_port = " + std::to_string(config.getInt("server_port")));
        LOG_INFO("  video_file = " + config.getString("video_file", "N/A"));
        LOG_INFO("  debug_mode = " +
                 std::string(config.getBool("debug_mode", false) ? "true" : "false"));

        // ==================== PLACEHOLDER FOR DAY 2+ ====================

        LOG_INFO("\n=== Day 2+ Implementation ===");
        LOG_INFO("[ ] RTSPClient will connect to " + serverIP);
        LOG_INFO("[ ] Send SETUP for video streaming");
        LOG_INFO("[ ] Send PLAY to start receiving RTP");
        LOG_INFO("[ ] RTPReceiver will receive video packets");
        LOG_INFO("[ ] Display video frames");

        // ==================== SUCCESS ====================

        LOG_INFO("\n========================================");
        LOG_INFO("   ✅ Day 1 Skeleton COMPLETED!");
        LOG_INFO("   Ready for RTSP implementation");
        LOG_INFO("========================================");
    } catch (const std::exception& e) {
        LOG_ERROR("FATAL ERROR: " + std::string(e.what()));
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}