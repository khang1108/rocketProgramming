/**
 * @file test_server_simulation.cpp
 * @brief Integration test: Simulate server sending RTP packets from video file
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

// Server components FIRST (includes RTPPacket from server)
#include "video/VideoStream.hpp"
#include "rtp/EncodingStrategy.hpp"
#include "rtp/RTPPacket.hpp"  // Use server's RTPPacket

// Client components (WITHOUT RTPPacket - already included from server)
#include "buffer/FrameBuffer.hpp"
// #include "rtp/FrameReassembler.hpp"  // This includes client's RTPPacket - conflict!

// Forward declare FrameReassembler to avoid including its header
class FrameReassembler {
public:
    explicit FrameReassembler(class FrameBuffer* frameBuffer);
    void addPacket(const RTPPacket& packet);
    size_t getPendingFrameCount() const;
private:
    // Don't need implementation here, will link against compiled .o file
};

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
        throw std::runtime_error(std::string("Assertion failed: ") + message + \
            " (expected " + std::to_string((size_t)(b)) + ", got " + std::to_string((size_t)(a)) + ")"); \
    }

// ==================== Helper Functions ====================

void createMockMJPEGFile(const std::string& filename, int numFrames) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create mock MJPEG file");
    }
    
    std::vector<uint8_t> jpegData = {
        0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10,
        0x4A, 0x46, 0x49, 0x46, 0x00,
        0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x00, 0xFF, 0xD9
    };
    
    for (int i = 0; i < numFrames; i++) {
        uint32_t length = static_cast<uint32_t>(jpegData.size());
        file.put(static_cast<char>(length & 0xFF));
        file.put(static_cast<char>((length >> 8) & 0xFF));
        file.put(static_cast<char>((length >> 16) & 0xFF));
        file.put(static_cast<char>((length >> 24) & 0xFF));
        file.put(0);
        file.write(reinterpret_cast<const char*>(jpegData.data()), jpegData.size());
    }
    
    file.close();
    std::cout << "\n  Created mock MJPEG: " << filename << " (" << numFrames << " frames)";
}

// ==================== Test Cases ====================

TEST(VideoStream_ReadMockFile) {
    std::string mockFile = "test_mock.Mjpeg";
    createMockMJPEGFile(mockFile, 3);
    
    VideoStream stream(mockFile);
    
    int frameCount = 0;
    while (stream.hasMoreFrames() && frameCount < 5) {
        auto frame = stream.nextFrame();
        ASSERT(frame.size() > 0, "Frame should not be empty");
        ASSERT_EQ(frame[0], 0xFF, "JPEG should start with 0xFF");
        ASSERT_EQ(frame[1], 0xD8, "JPEG SOI marker");
        frameCount++;
        std::cout << "\n    Frame " << frameCount << ": " << frame.size() << " bytes";
    }
    
    ASSERT_EQ(frameCount, 3, "Should read 3 frames");
    std::remove(mockFile.c_str());
}

TEST(EncodingStrategy_FragmentFrame) {
    EncodingContext encodingCtx;
    
    std::vector<uint8_t> largeFrame(3000, 0xAB);
    largeFrame[0] = 0xFF;
    largeFrame[1] = 0xD8;
    
    uint16_t startSeq = 100;
    uint32_t timestamp = 90000;
    uint32_t ssrc = 12345;
    
    Frame frame(largeFrame, startSeq, timestamp, ssrc);
    auto packets = encodingCtx.encodeFrame(frame);
    
    std::cout << "\n    Large frame (" << largeFrame.size() << " bytes) "
              << "→ " << packets.size() << " RTP packets";
    
    ASSERT(packets.size() > 1, "Large frame should be fragmented");
    
    for (size_t i = 0; i < packets.size(); i++) {
        const auto& pkt = packets[i];
        ASSERT_EQ(pkt.getSequenceNumber(), startSeq + i, "Sequence number");
        ASSERT_EQ(pkt.getTimestamp(), timestamp, "Timestamp");
        
        if (i == packets.size() - 1) {
            ASSERT_EQ(pkt.getMarker(), 1, "Last packet marker");
        } else {
            ASSERT_EQ(pkt.getMarker(), 0, "Non-last packet marker");
        }
        
        std::cout << "\n      Pkt " << i << ": seq=" << pkt.getSequenceNumber()
                  << ", payload=" << pkt.getPayloadSize() << " bytes, M=" 
                  << (int)pkt.getMarker();
    }
}

TEST(Integration_ServerToClient_SingleFrame) {
    std::cout << "\n  Pipeline: Server → RTP → FrameReassembler → FrameBuffer";
    
    FrameBuffer frameBuffer(10);
    FrameReassembler reassembler(&frameBuffer);
    
    EncodingContext encodingCtx;
    
    std::vector<uint8_t> originalFrame(500, 0x42);
    originalFrame[0] = 0xFF;
    originalFrame[1] = 0xD8;
    
    uint16_t seq = 1;
    uint32_t ts = 1000;
    uint32_t ssrc = 54321;
    
    Frame frame(originalFrame, seq, ts, ssrc);
    auto rtpPackets = encodingCtx.encodeFrame(frame);
    
    std::cout << "\n    Server: " << originalFrame.size() 
              << " bytes → " << rtpPackets.size() << " packets";
    
    for (const auto& pkt : rtpPackets) {
        auto bytes = pkt.getPacketVector();
        RTPPacket receivedPkt;
        receivedPkt.decode(bytes.data(), bytes.size());
        reassembler.addPacket(receivedPkt);
    }
    
    std::cout << "\n    Client: Received " << rtpPackets.size() << " packets";
    
    ASSERT_EQ(frameBuffer.size(), (size_t)1, "Frame in buffer");
    
    std::vector<uint8_t> receivedFrame;
    bool success = frameBuffer.pop(receivedFrame, 100);
    ASSERT(success, "Pop frame");
    ASSERT_EQ(receivedFrame.size(), originalFrame.size(), "Frame size");
    ASSERT_EQ(receivedFrame[0], 0xFF, "Frame data");
    
    std::cout << "\n    Client: Reassembled " << receivedFrame.size() << " bytes ✓";
}

TEST(Integration_ServerToClient_MultipleFrames) {
    std::cout << "\n  Testing: Multiple frames";
    
    FrameBuffer frameBuffer(10);
    FrameReassembler reassembler(&frameBuffer);
    EncodingContext encodingCtx;
    
    const int NUM_FRAMES = 5;
    uint16_t seq = 1;
    uint32_t ssrc = 99999;
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        std::vector<uint8_t> frameData(200 + i * 50, 0x10 + i);
        frameData[0] = 0xFF;
        frameData[1] = 0xD8;
        
        uint32_t timestamp = 1000 + i * 3600;
        
        Frame frame(frameData, seq, timestamp, ssrc);
        auto rtpPackets = encodingCtx.encodeFrame(frame);
        
        for (const auto& pkt : rtpPackets) {
            auto bytes = pkt.getPacketVector();
            RTPPacket receivedPkt;
            receivedPkt.decode(bytes.data(), bytes.size());
            reassembler.addPacket(receivedPkt);
        }
        
        seq += rtpPackets.size();
        
        std::cout << "\n    Frame " << (i+1) << ": " << frameData.size() 
                  << " bytes → " << rtpPackets.size() << " pkts";
    }
    
    ASSERT_EQ(frameBuffer.size(), (size_t)NUM_FRAMES, "All frames in buffer");
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        std::vector<uint8_t> frame;
        bool success = frameBuffer.pop(frame, 100);
        ASSERT(success, "Pop frame");
        ASSERT_EQ(frame.size(), (size_t)(200 + i * 50), "Frame size");
        std::cout << "\n    Received frame " << (i+1) << ": " << frame.size() << " bytes ✓";
    }
}

TEST(Integration_RealVideoFile) {
    std::cout << "\n  Testing: Real MJPEG file";
    
    std::string videoPath = "../../doc/Socket_2526/skeleton_python_rtp/python_rtp/movie.Mjpeg";
    
    std::ifstream checkFile(videoPath);
    if (!checkFile.good()) {
        std::cout << " [SKIPPED - file not found]";
        return;
    }
    checkFile.close();
    
    VideoStream stream(videoPath);
    FrameBuffer frameBuffer(30);
    FrameReassembler reassembler(&frameBuffer);
    EncodingContext encodingCtx;
    
    const int MAX_FRAMES = 10;
    int frameCount = 0;
    uint16_t seq = 1;
    uint32_t ssrc = 11111;
    
    while (stream.hasMoreFrames() && frameCount < MAX_FRAMES) {
        auto frameData = stream.nextFrame();
        uint32_t timestamp = frameCount * 3600;
        
        Frame frame(frameData, seq, timestamp, ssrc);
        auto rtpPackets = encodingCtx.encodeFrame(frame);
        
        for (const auto& pkt : rtpPackets) {
            auto bytes = pkt.getPacketVector();
            RTPPacket receivedPkt;
            receivedPkt.decode(bytes.data(), bytes.size());
            reassembler.addPacket(receivedPkt);
        }
        
        seq += rtpPackets.size();
        frameCount++;
        
        if (frameCount % 5 == 0) {
            std::cout << "\n    Streamed " << frameCount << " frames, buffer: " 
                      << frameBuffer.size();
        }
    }
    
    std::cout << "\n    Total: " << frameCount << " frames";
    ASSERT(frameBuffer.size() > 0, "Frames in buffer");
    ASSERT_EQ(frameBuffer.size(), (size_t)frameCount, "All frames received");
}

TEST(Performance_Throughput) {
    std::cout << "\n  Testing: Throughput";
    
    FrameBuffer frameBuffer(100);
    FrameReassembler reassembler(&frameBuffer);
    EncodingContext encodingCtx;
    
    const int NUM_FRAMES = 100;
    const size_t FRAME_SIZE = 5000;
    
    auto startTime = std::chrono::steady_clock::now();
    
    uint16_t seq = 1;
    uint32_t ssrc = 77777;
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        std::vector<uint8_t> frameData(FRAME_SIZE, 0xAA);
        frameData[0] = 0xFF;
        frameData[1] = 0xD8;
        
        uint32_t timestamp = i * 3600;
        
        Frame frame(frameData, seq, timestamp, ssrc);
        auto rtpPackets = encodingCtx.encodeFrame(frame);
        
        for (const auto& pkt : rtpPackets) {
            auto bytes = pkt.getPacketVector();
            RTPPacket receivedPkt;
            receivedPkt.decode(bytes.data(), bytes.size());
            reassembler.addPacket(receivedPkt);
        }
        
        seq += rtpPackets.size();
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    double mbytes = (NUM_FRAMES * FRAME_SIZE) / (1024.0 * 1024.0);
    double mbps = (mbytes * 8 * 1000) / elapsed;
    
    std::cout << "\n    " << NUM_FRAMES << " frames (" << mbytes << " MB) in " 
              << elapsed << " ms";
    std::cout << "\n    Throughput: " << mbps << " Mbps";
    
    ASSERT_EQ(frameBuffer.size(), (size_t)NUM_FRAMES, "All frames processed");
}

// ==================== Main ====================

int main() {
    std::cout << "\n========================================\n";
    std::cout << "Server Simulation & Integration Tests\n";
    std::cout << "========================================\n";
    
    test_VideoStream_ReadMockFile_wrapper();
    test_EncodingStrategy_FragmentFrame_wrapper();
    test_Integration_ServerToClient_SingleFrame_wrapper();
    test_Integration_ServerToClient_MultipleFrames_wrapper();
    test_Integration_RealVideoFile_wrapper();
    test_Performance_Throughput_wrapper();
    
    std::cout << "\n========================================\n";
    std::cout << "Results: ";
    if (g_testsFailed == 0) {
        std::cout << "ALL PASSED ✓\n";
    } else {
        std::cout << g_testsFailed << " FAILED ✗\n";
    }
    std::cout << "Passed: " << g_testsPassed << "/" << (g_testsPassed + g_testsFailed) << "\n";
    std::cout << "========================================\n\n";
    
    return (g_testsFailed == 0) ? 0 : 1;
}