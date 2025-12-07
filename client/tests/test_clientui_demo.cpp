/**
 * @file test_clientui_demo.cpp
 * @brief Interactive demo for ClientUI with mock RTSP server
 */

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "ui/ClientUI.hpp"
#include "network/Socket.hpp"
#include "utils/Logger.hpp"

// ==================== MOCK RTSP SERVER ====================

class MockRTSPServer {
private:
    std::unique_ptr<Socket> serverSocket_;
    std::unique_ptr<Socket> clientSocket_;
    int port_;
    std::thread serverThread_;
    std::thread rtpThread_;
    std::atomic<bool> running_;
    std::atomic<bool> streaming_;
    std::string sessionId_;
    std::string clientIP_;
    int clientRTPPort_;
    
public:
    MockRTSPServer(int port = 8554) 
        : port_(port), running_(false), streaming_(false), 
          sessionId_("DEMO123456"), clientRTPPort_(0) {}
    
    ~MockRTSPServer() { stop(); }
    
    void start() {
        running_ = true;
        
        // Create server socket
        serverSocket_ = std::make_unique<Socket>(SocketType::TCP);
        serverSocket_->setReuseAddress(true);
        serverSocket_->bind("127.0.0.1", port_);
        serverSocket_->listen(1);
        
        std::cout << "[Mock Server] Listening on port " << port_ << "\n";
        
        serverThread_ = std::thread(&MockRTSPServer::serverLoop, this);
    }
    
    void stop() {
        running_ = false;
        streaming_ = false;
        
        if (clientSocket_) {
            try { clientSocket_->close(); } catch (...) {}
        }
        if (serverSocket_) {
            try { serverSocket_->close(); } catch (...) {}
        }
        
        if (rtpThread_.joinable()) rtpThread_.join();
        if (serverThread_.joinable()) serverThread_.join();
    }
    
private:
    void serverLoop() {
        try {
            // Accept one client
            std::cout << "[Mock Server] Waiting for client...\n";
            clientSocket_ = serverSocket_->accept();
            std::cout << "[Mock Server] Client connected!\n";
            
            clientIP_ = clientSocket_->getPeerAddress();
            
            while (running_) {
                // Read RTSP request
                std::vector<uint8_t> buffer(4096);
                int bytes = clientSocket_->receive(buffer.data(), buffer.size());
                if (bytes <= 0) break;
                
                std::string request(reinterpret_cast<char*>(buffer.data()), bytes);
                std::cout << "[Mock Server] Received request:\n" << request << "\n";
                
                // Handle request
                std::string response;
                if (request.find("SETUP") != std::string::npos) {
                    response = handleSetup(request);
                } else if (request.find("PLAY") != std::string::npos) {
                    response = handlePlay(request);
                } else if (request.find("PAUSE") != std::string::npos) {
                    response = handlePause(request);
                } else if (request.find("TEARDOWN") != std::string::npos) {
                    response = handleTeardown(request);
                    clientSocket_->send(reinterpret_cast<const uint8_t*>(response.data()), response.size());
                    break;
                } else {
                    response = "RTSP/1.0 400 Bad Request\r\n\r\n";
                }
                
                // Send response
                clientSocket_->send(reinterpret_cast<const uint8_t*>(response.data()), response.size());
            }
        } catch (const std::exception& e) {
            std::cerr << "[Mock Server] Error: " << e.what() << "\n";
        }
    }
    
    std::string handleSetup(const std::string& request) {
        int cseq = extractCSeq(request);
        
        // Extract client_port
        auto pos = request.find("client_port=");
        if (pos != std::string::npos) {
            clientRTPPort_ = std::stoi(request.substr(pos + 12));
            std::cout << "[Mock Server] Client RTP port: " << clientRTPPort_ << "\n";
        }
        
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "Session: " << sessionId_ << "\r\n";
        resp << "Transport: RTP/UDP; client_port=" << clientRTPPort_ 
             << "; server_port=25000\r\n";
        resp << "\r\n";
        
        std::cout << "[Mock Server] SETUP OK\n";
        return resp.str();
    }
    
    std::string handlePlay(const std::string& request) {
        int cseq = extractCSeq(request);
        
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "Session: " << sessionId_ << "\r\n";
        resp << "\r\n";
        
        // Start streaming mock RTP packets
        streaming_ = true;
        rtpThread_ = std::thread(&MockRTSPServer::streamMockVideo, this);
        
        std::cout << "[Mock Server] PLAY OK - Starting mock RTP stream\n";
        return resp.str();
    }
    
    std::string handlePause(const std::string& request) {
        int cseq = extractCSeq(request);
        streaming_ = false;
        
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "Session: " << sessionId_ << "\r\n";
        resp << "\r\n";
        
        std::cout << "[Mock Server] PAUSE OK\n";
        return resp.str();
    }
    
    std::string handleTeardown(const std::string& request) {
        int cseq = extractCSeq(request);
        streaming_ = false;
        
        std::ostringstream resp;
        resp << "RTSP/1.0 200 OK\r\n";
        resp << "CSeq: " << cseq << "\r\n";
        resp << "\r\n";
        
        std::cout << "[Mock Server] TEARDOWN OK\n";
        return resp.str();
    }
    
    void streamMockVideo() {
        std::cout << "[Mock Server] Mock RTP streaming started\n";
        
        // Create UDP socket for RTP
        auto rtpSocket = std::make_unique<Socket>(SocketType::UDP);
        
        // Create a simple test pattern JPEG (minimal valid JPEG)
        std::vector<uint8_t> testJPEG = {
            0xFF, 0xD8,  // SOI
            0xFF, 0xE0,  // APP0
            0x00, 0x10,  // Length
            0x4A, 0x46, 0x49, 0x46, 0x00,  // "JFIF"
            0x01, 0x01,  // Version
            0x00,        // Units
            0x00, 0x01, 0x00, 0x01,  // Density
            0x00, 0x00,  // Thumbnail
            // Add some color data
            0xFF, 0xDB,  // DQT
            0x00, 0x43,  // Length
        };
        
        // Pad to make a reasonable size
        testJPEG.resize(500, 0x00);
        testJPEG.push_back(0xFF);
        testJPEG.push_back(0xD9);  // EOI
        
        uint16_t seq = 1;
        uint32_t timestamp = 0;
        
        while (streaming_) {
            // Create simple RTP packet
            std::vector<uint8_t> rtpPacket;
            
            // RTP Header (12 bytes)
            rtpPacket.push_back(0x80);  // V=2, P=0, X=0, CC=0
            rtpPacket.push_back(0x9A);  // M=1, PT=26 (MJPEG)
            rtpPacket.push_back((seq >> 8) & 0xFF);
            rtpPacket.push_back(seq & 0xFF);
            rtpPacket.push_back((timestamp >> 24) & 0xFF);
            rtpPacket.push_back((timestamp >> 16) & 0xFF);
            rtpPacket.push_back((timestamp >> 8) & 0xFF);
            rtpPacket.push_back(timestamp & 0xFF);
            rtpPacket.push_back(0x00); rtpPacket.push_back(0x00);
            rtpPacket.push_back(0x00); rtpPacket.push_back(0x01);  // SSRC
            
            // Add payload
            rtpPacket.insert(rtpPacket.end(), testJPEG.begin(), testJPEG.end());
            
            // Send to client
            try {
                rtpSocket->sendTo(rtpPacket.data(), rtpPacket.size(),
                                 clientIP_, clientRTPPort_);
                
                if (seq % 25 == 0) {
                    std::cout << "[Mock Server] Sent " << seq << " RTP packets\n";
                }
            } catch (...) {
                break;
            }
            
            seq++;
            timestamp += 3600;  // 40ms per frame @ 90kHz
            
            // 25 fps = 40ms per frame
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
        
        std::cout << "[Mock Server] Mock RTP streaming stopped\n";
    }
    
    int extractCSeq(const std::string& request) {
        auto pos = request.find("CSeq:");
        if (pos != std::string::npos) {
            return std::stoi(request.substr(pos + 5));
        }
        return 0;
    }
};

// ==================== MAIN DEMO ====================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "   ClientUI Interactive Demo\n";
    std::cout << "========================================\n\n";
    
    try {
        // Initialize logger
        Logger::getInstance().initialize("clientui_demo.log", LogLevel::INFO);
        
        // Start mock server
        std::cout << "Starting mock RTSP server...\n";
        MockRTSPServer mockServer(8554);
        mockServer.start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Create ClientUI
        std::cout << "\nCreating ClientUI...\n";
        ClientUI clientUI("127.0.0.1", 8554, "movie.Mjpeg", 25000);
        
        // Initialize
        std::cout << "Initializing ClientUI...\n";
        if (!clientUI.initialize()) {
            std::cerr << "Failed to initialize ClientUI\n";
            return 1;
        }
        
        std::cout << "\n========================================\n";
        std::cout << "ClientUI Demo Running!\n";
        std::cout << "========================================\n";
        std::cout << "\nInstructions:\n";
        std::cout << "  1. Click 'SETUP' button to establish session\n";
        std::cout << "  2. Click 'PLAY' button to start video\n";
        std::cout << "  3. Click 'PAUSE' button to pause\n";
        std::cout << "  4. Click 'TEARDOWN' or close window to exit\n";
        std::cout << "\nNote: This is a DEMO with mock video frames\n";
        std::cout << "========================================\n\n";
        
        // Run UI (blocking until user closes)
        clientUI.run();
        
        // Cleanup
        std::cout << "\nShutting down demo...\n";
        mockServer.stop();
        
        std::cout << "Demo completed successfully!\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}