/**
 * @file test_rtsp_client.cpp
 * @brief Comprehensive test suite for RTSPClient
 */

#include <iostream>
#include <string>
#include <cassert>
#include <sstream>
#include <atomic>
#include <memory>
#include <thread>
#include "network/RTSPClient.hpp"

// Test utilities
int g_testsPassed = 0;
int g_testsFailed = 0;

#define TEST(name) \
    void test_##name(); \
    void test_##name##_wrapper() { \
        std::cout << "\n[TEST] " << #name << " ... "; \
        try { \
            test_##name(); \
            std::cout << "PASSED"; \
            g_testsPassed++; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what(); \
            g_testsFailed++; \
        } \
    } \
    void test_##name()

#define ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + message); \
    }

#define ASSERT_EQ(a, b, message) \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + message); \
    }

// ==================== HELPER: Mock RTSP Server ====================

/**
 * @brief Mock RTSP server for testing
 * 
 * This class simulates a simple RTSP server that responds to:
 * - SETUP: Returns session ID
 * - PLAY: Returns 200 OK
 * - PAUSE: Returns 200 OK  
 * - TEARDOWN: Returns 200 OK
 */
class MockRTSPServer {
private:
    std::unique_ptr<Socket> serverSocket_;
    std::unique_ptr<Socket> clientSocket_;
    int port_;
    std::thread serverThread_;
    std::atomic<bool> running_;
    std::string sessionId_;
    
public:
    MockRTSPServer(int port = 8554) : port_(port), running_(false), sessionId_("TEST123456") {
        serverSocket_ = std::make_unique<Socket>(SocketType::TCP);
        serverSocket_->setReuseAddress(true);
        serverSocket_->bind("127.0.0.1", port_);
        serverSocket_->listen(1);
    }
    
    ~MockRTSPServer() {
        stop();
    }
    
    void start() {
        running_ = true;
        serverThread_ = std::thread(&MockRTSPServer::serverLoop, this);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give server time to start
    }
    
    void stop() {
        running_ = false;
        if (clientSocket_) {
            clientSocket_->close();
        }
        if (serverSocket_) {
            serverSocket_->close();
        }
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }
    
private:
    void serverLoop() {
        try {
            // Accept one client
            clientSocket_ = serverSocket_->accept();
            
            while (running_) {
                // Read request
                std::vector<uint8_t> buffer(4096);
                int bytes = clientSocket_->receive(buffer.data(), buffer.size());
                if (bytes <= 0) break;
                
                std::string request(reinterpret_cast<char*>(buffer.data()), bytes);
                
                // Parse method
                std::string response;
                if (request.find("SETUP") != std::string::npos) {
                    response = handleSetup(request);
                } else if (request.find("PLAY") != std::string::npos) {
                    response = handlePlay(request);
                } else if (request.find("PAUSE") != std::string::npos) {
                    response = handlePause(request);
                } else if (request.find("TEARDOWN") != std::string::npos) {
                    response = handleTeardown(request);
                } else {
                    response = "RTSP/1.0 400 Bad Request\r\n\r\n";
                }
                
                // Send response
                clientSocket_->send(reinterpret_cast<const uint8_t*>(response.data()), 
                            response.size());
            }
        } catch (...) {
            // Server error - stop
        }
    }
    
    std::string handleSetup(const std::string& request) {
        // Extract CSeq
        int cseq = extractCSeq(request);
        
        // Extract client_port
        int clientPort = 25000;
        auto pos = request.find("client_port=");
        if (pos != std::string::npos) {
            clientPort = std::stoi(request.substr(pos + 12));
        }
        
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "Session: " << sessionId_ << "\r\n";
        resp << "Transport: RTP/UDP; client_port=" << clientPort 
             << "; server_port=25000\r\n";
        resp << "\r\n";
        return resp.str();
    }
    
    std::string handlePlay(const std::string& request) {
        int cseq = extractCSeq(request);
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "Session: " << sessionId_ << "\r\n";
        resp << "\r\n";
        return resp.str();
    }
    
    std::string handlePause(const std::string& request) {
        int cseq = extractCSeq(request);
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "Session: " << sessionId_ << "\r\n";
        resp << "\r\n";
        return resp.str();
    }
    
    std::string handleTeardown(const std::string& request) {
        int cseq = extractCSeq(request);
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "\r\n";
        return resp.str();
    }
    
    int extractCSeq(const std::string& request) {
        auto pos = request.find("CSeq:");
        if (pos != std::string::npos) {
            return std::stoi(request.substr(pos + 5));
        }
        return 0;
    }
};

// ==================== TEST CASES ====================

TEST(Constructor_ValidServer) {
    MockRTSPServer server(8554);
    server.start();
    
    // Create client and connect
    RTSPClient client("127.0.0.1", 8554);
    
    // Verify initial state
    ASSERT_EQ(client.getState(), RTSPClient::State::INIT, "Initial state should be INIT");
    ASSERT(client.getSessionId().empty(), "Session ID should be empty initially");
    ASSERT_EQ(client.getStateString(), "INIT", "State string should be INIT");
    
    std::cout << "\n    Client connected successfully";
    std::cout << "\n    Initial state: " << client.getStateString();
}

TEST(SendSetup_Success) {
    MockRTSPServer server(8555);
    server.start();
    
    RTSPClient client("127.0.0.1", 8555);
    
    // Send SETUP
    bool success = client.sendSetup("movie.Mjpeg", 25000);
    
    ASSERT(success, "SETUP should succeed");
    ASSERT_EQ(client.getState(), RTSPClient::State::READY, "State should transition to READY");
    ASSERT(!client.getSessionId().empty(), "Session ID should be set");
    ASSERT_EQ(client.getClientRTPPort(), 25000, "Client RTP port should be stored");
    ASSERT_EQ(client.getServerRTPPort(), 25000, "Server RTP port should be extracted");
    
    std::cout << "\n    SETUP successful";
    std::cout << "\n    Session ID: " << client.getSessionId();
    std::cout << "\n    State: " << client.getStateString();
}

TEST(SendSetup_WrongState) {
    MockRTSPServer server(8556);
    server.start();
    
    RTSPClient client("127.0.0.1", 8556);
    
    // First SETUP - should succeed
    client.sendSetup("movie.Mjpeg", 25000);
    
    // Second SETUP - should fail (wrong state)
    bool success = client.sendSetup("movie.Mjpeg", 25000);
    
    ASSERT(!success, "Second SETUP should fail (not in INIT state)");
    std::cout << "\n    Correctly rejected SETUP in wrong state";
}

TEST(SendPlay_Success) {
    MockRTSPServer server(8557);
    server.start();
    
    RTSPClient client("127.0.0.1", 8557);
    
    // SETUP first
    client.sendSetup("movie.Mjpeg", 25000);
    
    // PLAY
    bool success = client.sendPlay();
    
    ASSERT(success, "PLAY should succeed");
    ASSERT_EQ(client.getState(), RTSPClient::State::PLAYING, "State should transition to PLAYING");
    
    std::cout << "\n    PLAY successful";
    std::cout << "\n    State: " << client.getStateString();
}

TEST(SendPlay_WrongState) {
    MockRTSPServer server(8558);
    server.start();
    
    RTSPClient client("127.0.0.1", 8558);
    
    // Try PLAY without SETUP
    bool success = client.sendPlay();
    
    ASSERT(!success, "PLAY should fail without SETUP");
    ASSERT_EQ(client.getState(), RTSPClient::State::INIT, "State should remain INIT");
    
    std::cout << "\n    Correctly rejected PLAY in INIT state";
}

TEST(SendPause_Success) {
    MockRTSPServer server(8559);
    server.start();
    
    RTSPClient client("127.0.0.1", 8559);
    
    // SETUP -> PLAY -> PAUSE
    client.sendSetup("movie.Mjpeg", 25000);
    client.sendPlay();
    
    bool success = client.sendPause();
    
    ASSERT(success, "PAUSE should succeed");
    ASSERT_EQ(client.getState(), RTSPClient::State::READY, "State should transition back to READY");
    
    std::cout << "\n    PAUSE successful";
    std::cout << "\n    State: " << client.getStateString();
}

TEST(SendTeardown_Success) {
    MockRTSPServer server(8560);
    server.start();
    
    RTSPClient client("127.0.0.1", 8560);
    
    // SETUP -> PLAY -> TEARDOWN
    client.sendSetup("movie.Mjpeg", 25000);
    client.sendPlay();
    
    bool success = client.sendTeardown();
    
    ASSERT(success, "TEARDOWN should succeed");
    ASSERT_EQ(client.getState(), RTSPClient::State::INIT, "State should return to INIT");
    ASSERT(client.getSessionId().empty(), "Session ID should be cleared");
    
    std::cout << "\n    TEARDOWN successful";
    std::cout << "\n    Session cleared";
}

TEST(StateMachine_FullSequence) {
    MockRTSPServer server(8561);
    server.start();
    
    RTSPClient client("127.0.0.1", 8561);
    
    // Full sequence: INIT -> SETUP -> READY -> PLAY -> PLAYING -> PAUSE -> READY -> TEARDOWN -> INIT
    std::cout << "\n    Testing full state machine sequence:";
    
    // INIT
    ASSERT_EQ(client.getState(), RTSPClient::State::INIT, "Start in INIT");
    std::cout << "\n      INIT ✓";
    
    // SETUP
    client.sendSetup("movie.Mjpeg", 25000);
    ASSERT_EQ(client.getState(), RTSPClient::State::READY, "SETUP -> READY");
    std::cout << "\n      SETUP -> READY ✓";
    
    // PLAY
    client.sendPlay();
    ASSERT_EQ(client.getState(), RTSPClient::State::PLAYING, "PLAY -> PLAYING");
    std::cout << "\n      PLAY -> PLAYING ✓";
    
    // PAUSE
    client.sendPause();
    ASSERT_EQ(client.getState(), RTSPClient::State::READY, "PAUSE -> READY");
    std::cout << "\n      PAUSE -> READY ✓";
    
    // TEARDOWN
    client.sendTeardown();
    ASSERT_EQ(client.getState(), RTSPClient::State::INIT, "TEARDOWN -> INIT");
    std::cout << "\n      TEARDOWN -> INIT ✓";
}

TEST(ParseStatusCode) {
    MockRTSPServer server(8562);
    server.start();
    
    RTSPClient client("127.0.0.1", 8562);
    
    // This test verifies status code parsing by sending requests
    client.sendSetup("movie.Mjpeg", 25000);
    
    // If SETUP succeeded, status code 200 was correctly parsed
    ASSERT_EQ(client.getState(), RTSPClient::State::READY, "Status code 200 parsed correctly");
    
    std::cout << "\n    Status code parsing works correctly";
}

TEST(ExtractSessionId) {
    MockRTSPServer server(8563);
    server.start();
    
    RTSPClient client("127.0.0.1", 8563);
    
    client.sendSetup("movie.Mjpeg", 25000);
    
    std::string sessionId = client.getSessionId();
    ASSERT(!sessionId.empty(), "Session ID should be extracted");
    ASSERT_EQ(sessionId, "TEST123456", "Session ID should match server's");
    
    std::cout << "\n    Session ID extracted: " << sessionId;
}

TEST(Destructor_AutoTeardown) {
    MockRTSPServer server(8564);
    server.start();
    
    {
        RTSPClient client("127.0.0.1", 8564);
        client.sendSetup("movie.Mjpeg", 25000);
        client.sendPlay();
        
        // Client goes out of scope - should auto TEARDOWN
    }
    
    std::cout << "\n    Destructor auto-teardown executed";
    // If no crash/hang, test passed
}

// ==================== MAIN ====================

int main() {
    std::cout << "\n========================================\n";
    std::cout << "RTSPClient Test Suite\n";
    std::cout << "========================================\n";
    
    test_Constructor_ValidServer_wrapper();
    test_SendSetup_Success_wrapper();
    test_SendSetup_WrongState_wrapper();
    test_SendPlay_Success_wrapper();
    test_SendPlay_WrongState_wrapper();
    test_SendPause_Success_wrapper();
    test_SendTeardown_Success_wrapper();
    test_StateMachine_FullSequence_wrapper();
    test_ParseStatusCode_wrapper();
    test_ExtractSessionId_wrapper();
    test_Destructor_AutoTeardown_wrapper();
    
    std::cout << "\n\n========================================\n";
    std::cout << "Results: ";
    if (g_testsFailed == 0) {
        std::cout << "ALL TESTS PASSED ✓\n";
    } else {
        std::cout << g_testsFailed << " TESTS FAILED ✗\n";
    }
    std::cout << "Passed: " << g_testsPassed << "/" << (g_testsPassed + g_testsFailed) << "\n";
    std::cout << "========================================\n\n";
    
    return (g_testsFailed == 0) ? 0 : 1;
}