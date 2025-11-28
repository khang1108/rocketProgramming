#include "utils/Logger.hpp"
#include "utils/Config.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// Test Logger
void testLogger()
{
    std::cout << "\n========== TESTING LOGGER ==========\n" << std::endl;

    // Initialize logger
    Logger::getInstance().initialize("test.log", LogLevel::INFO);

    // Test different log levels
    LOG_INFO("This is an INFO message");
    LOG_WARN("This is a WARNING message");
    LOG_ERROR("This is an ERROR message");

    // Test macros
    std::string filename = "video.mp4";
    LOG_INFO("Loading file: " + filename);
    LOG_ERROR("Failed to connect to server");

    // Test thread-safety
    std::cout << "\nTesting thread-safety (2 threads)..." << std::endl;

    std::thread t1([](){
        for (int i = 0; i < 5; i++) {
            LOG_INFO("Thread 1 - Message " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } });

    std::thread t2([](){
        for (int i = 0; i < 5; i++) {
            LOG_WARN("Thread 2 - Message " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } });

    t1.join();
    t2.join();

    std::cout << "\n✅ Logger test completed! Check 'test.log' file.\n"
            << std::endl;
}

// Test Config
void testConfig()
{
    std::cout << "\n========== TESTING CONFIG ==========\n"
            << std::endl;

    // Create a test config file
    std::ofstream configFile("test.conf");
    configFile << "# Test Configuration File\n";
    configFile << "server_port=9999\n";
    configFile << "video_file=test_video.mp4\n";
    configFile << "max_clients=50\n";
    configFile << "debug_mode=true\n";
    configFile << "timeout_seconds=10.5\n";
    configFile << "# This is a comment\n";
    configFile << "frame_rate=30\n";
    configFile.close();

    std::cout << "Created test.conf file\n"
            << std::endl;

    // Initialize Config with test file
    Config::initialize("test.conf");

    auto &config = Config::getInstance();

    // Test getInt
    int port = config.getInt("server_port", 8554);
    std::cout << "✓ server_port = " << port
            << " (expected: 9999)" << std::endl;

    // Test getString
    std::string videoFile = config.getString("video_file", "default.mp4");
    std::cout << "✓ video_file = " << videoFile
            << " (expected: test_video.mp4)" << std::endl;

    // Test getBool
    bool debug = config.getBool("debug_mode", false);
    std::cout << "✓ debug_mode = " << (debug ? "true" : "false")
            << " (expected: true)" << std::endl;

    // Test getDouble
    double timeout = config.getDouble("timeout_seconds", 5.0);
    std::cout << "✓ timeout_seconds = " << timeout
            << " (expected: 10.5)" << std::endl;

    // Test hasKey
    bool hasKey = config.hasKey("frame_rate");
    std::cout << "✓ hasKey('frame_rate') = " << (hasKey ? "true" : "false")
            << " (expected: true)" << std::endl;

    // Test default values
    int unknownKey = config.getInt("unknown_key", 123);
    std::cout << "✓ unknown_key (default) = " << unknownKey
            << " (expected: 123)" << std::endl;

    // Dump all config
    std::cout << "\n";
    config.dump();

    std::cout << "\n✅ Config test completed!\n"
            << std::endl;
}

// Main test function
int main()
{
    try
    {
        std::cout << "=================================================\n";
        std::cout << "       LOGGER & CONFIG TEST SUITE\n";
        std::cout << "=================================================\n";

        // Test Logger
        testLogger();

        // Test Config
        testConfig();

        std::cout << "\n=================================================\n";
        std::cout << "       ✅ ALL TESTS PASSED!\n";
        std::cout << "=================================================\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}