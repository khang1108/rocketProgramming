/**
 * @file test_rtp_pipeline.cpp
 * @brief Comprehensive test for RTP receiving and frame reassembly pipeline
 */

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

// Include components to test
#include "buffer/FrameBuffer.hpp"
#include "rtp/FrameReassembler.hpp"
#include "rtp/RTPPacket.hpp"
// #include "rtp/RTPPacketBuilder.hpp"  // ❌ REMOVE - server only

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
            " (expected " + std::to_string(b) + ", got " + std::to_string(a) + ")"); \
    }

// Helper to create RTP packet manually (without builder)
RTPPacket createTestPacket(uint16_t seq, uint32_t timestamp, uint8_t marker, 
                          const std::vector<uint8_t>& payload) {
    RTPPacket packet;
    packet.setSequenceNumber(seq);
    packet.setTimestamp(timestamp);
    packet.setMarker(marker);
    packet.setPayload(payload);
    packet.encode();  // Encode header
    return packet;
}

// ==================== Test Cases ====================

TEST(FrameBuffer_PushPop) {
    FrameBuffer buffer(5);
    
    // Push frame
    std::vector<uint8_t> frame1 = {1, 2, 3, 4, 5};
    ASSERT(buffer.push(frame1), "Push failed");
    ASSERT_EQ(buffer.size(), 1, "Buffer size should be 1");
    
    // Pop frame
    std::vector<uint8_t> frameOut;
    ASSERT(buffer.pop(frameOut, 100), "Pop failed");
    ASSERT_EQ(frameOut.size(), 5, "Frame size mismatch");
    ASSERT_EQ(frameOut[0], 1, "Frame data mismatch");
    ASSERT_EQ(buffer.size(), 0, "Buffer should be empty");
}

TEST(FrameBuffer_BlockingBehavior) {
    FrameBuffer buffer(2);
    
    // Fill buffer
    std::vector<uint8_t> frame1 = {1, 2, 3};
    std::vector<uint8_t> frame2 = {4, 5, 6};
    buffer.push(frame1);
    buffer.push(frame2);
    
    ASSERT(buffer.isFull(), "Buffer should be full");
    
    // Pop in another thread
    std::thread consumer([&buffer]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::vector<uint8_t> frame;
        buffer.pop(frame, 1000);
    });
    
    // This should block briefly then succeed
    auto start = std::chrono::steady_clock::now();
    std::vector<uint8_t> frame3 = {7, 8, 9};
    buffer.push(frame3);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    ASSERT(elapsed >= 40, "Push should have blocked");
    
    consumer.join();
}

TEST(RTPPacket_EncodeDecode) {
    // Create packet manually
    std::vector<uint8_t> payload = {0xFF, 0xD8, 0xFF, 0xE0};  // JPEG header
    
    RTPPacket packet = createTestPacket(100, 3600, 1, payload);
    
    // Verify fields
    ASSERT_EQ(packet.getSequenceNumber(), 100, "Sequence number mismatch");
    ASSERT_EQ(packet.getTimestamp(), 3600, "Timestamp mismatch");
    ASSERT_EQ(packet.getMarker(), 1, "Marker bit mismatch");
    ASSERT_EQ(packet.getPayloadSize(), 4, "Payload size mismatch");
    
    // Encode to bytes
    auto packetBytes = packet.getPacketVector();
    ASSERT_EQ(packetBytes.size(), 16, "Packet size should be header(12) + payload(4)");
    
    // Decode back
    RTPPacket decoded;
    decoded.decode(packetBytes.data(), packetBytes.size());
    
    ASSERT_EQ(decoded.getSequenceNumber(), 100, "Decoded sequence mismatch");
    ASSERT_EQ(decoded.getTimestamp(), 3600, "Decoded timestamp mismatch");
    ASSERT_EQ(decoded.getMarker(), 1, "Decoded marker mismatch");
}

TEST(FrameReassembler_SinglePacket) {
    FrameBuffer buffer(10);
    FrameReassembler reassembler(&buffer);
    
    // Create single-packet frame
    std::vector<uint8_t> jpegData = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10};
    RTPPacket packet = createTestPacket(1, 1000, 1, jpegData);
    
    // Add packet
    reassembler.addPacket(packet);
    
    // Check frame in buffer
    ASSERT_EQ(buffer.size(), 1, "Frame should be in buffer");
    
    std::vector<uint8_t> frameOut;
    buffer.pop(frameOut, 100);
    ASSERT_EQ(frameOut.size(), 6, "Frame size mismatch");
    ASSERT_EQ(frameOut[0], 0xFF, "JPEG header mismatch");
    ASSERT_EQ(frameOut[1], 0xD8, "JPEG header mismatch");
}

TEST(FrameReassembler_MultiPacket_InOrder) {
    FrameBuffer buffer(10);
    FrameReassembler reassembler(&buffer);
    
    uint32_t timestamp = 2000;
    
    // Fragment 1
    RTPPacket pkt1 = createTestPacket(10, timestamp, 0, {0xFF, 0xD8});
    
    // Fragment 2
    RTPPacket pkt2 = createTestPacket(11, timestamp, 0, {0xFF, 0xE0});
    
    // Fragment 3 (last)
    RTPPacket pkt3 = createTestPacket(12, timestamp, 1, {0x00, 0x10});
    
    // Add in order
    reassembler.addPacket(pkt1);
    ASSERT_EQ(buffer.size(), 0, "Frame not complete yet");
    
    reassembler.addPacket(pkt2);
    ASSERT_EQ(buffer.size(), 0, "Frame not complete yet");
    
    reassembler.addPacket(pkt3);
    ASSERT_EQ(buffer.size(), 1, "Frame should be complete");
    
    // Verify reassembled frame
    std::vector<uint8_t> frameOut;
    buffer.pop(frameOut, 100);
    ASSERT_EQ(frameOut.size(), 6, "Reassembled frame size mismatch");
    ASSERT_EQ(frameOut[0], 0xFF, "Fragment 1 data");
    ASSERT_EQ(frameOut[1], 0xD8, "Fragment 1 data");
    ASSERT_EQ(frameOut[2], 0xFF, "Fragment 2 data");
    ASSERT_EQ(frameOut[3], 0xE0, "Fragment 2 data");
    ASSERT_EQ(frameOut[4], 0x00, "Fragment 3 data");
    ASSERT_EQ(frameOut[5], 0x10, "Fragment 3 data");
}

TEST(FrameReassembler_MultipleFrames) {
    FrameBuffer buffer(10);
    FrameReassembler reassembler(&buffer);
    
    // Frame 1 (timestamp 1000)
    RTPPacket frame1 = createTestPacket(1, 1000, 1, {0x01});
    
    // Frame 2 (timestamp 2000)
    RTPPacket frame2 = createTestPacket(2, 2000, 1, {0x02});
    
    // Frame 3 (timestamp 3000)
    RTPPacket frame3 = createTestPacket(3, 3000, 1, {0x03});
    
    reassembler.addPacket(frame1);
    reassembler.addPacket(frame2);
    reassembler.addPacket(frame3);
    
    ASSERT_EQ(buffer.size(), 3, "3 frames should be in buffer");
    
    // Pop in order
    std::vector<uint8_t> f1, f2, f3;
    buffer.pop(f1, 100);
    buffer.pop(f2, 100);
    buffer.pop(f3, 100);
    
    ASSERT_EQ(f1[0], 0x01, "Frame 1 data");
    ASSERT_EQ(f2[0], 0x02, "Frame 2 data");
    ASSERT_EQ(f3[0], 0x03, "Frame 3 data");
}

TEST(FrameReassembler_Cleanup) {
    FrameBuffer buffer(100);
    FrameReassembler reassembler(&buffer);
    
    // Add 70 incomplete frames (no marker bit)
    for (int i = 0; i < 70; i++) {
        RTPPacket pkt = createTestPacket(i, i * 1000, 0, 
            {static_cast<uint8_t>(i)});
        reassembler.addPacket(pkt);
    }
    
    // Should have cleaned up old frames (max 64)
    ASSERT(reassembler.getPendingFrameCount() <= 64, 
           "Should cleanup old incomplete frames");
}

TEST(FrameBuffer_Timeout) {
    FrameBuffer buffer(5);
    
    // Try to pop from empty buffer with timeout
    std::vector<uint8_t> frame;
    auto start = std::chrono::steady_clock::now();
    bool success = buffer.pop(frame, 100);  // 100ms timeout
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    ASSERT(!success, "Pop should timeout");
    ASSERT(elapsed >= 90 && elapsed <= 150, "Timeout duration incorrect");
}

TEST(FrameBuffer_Close) {
    FrameBuffer buffer(5);
    
    // Push some frames
    buffer.push(std::vector<uint8_t>{1, 2, 3});
    buffer.push(std::vector<uint8_t>{4, 5, 6});
    
    // Close buffer
    buffer.close();
    
    ASSERT(buffer.isClosed(), "Buffer should be closed");
    
    // Cannot push after close
    bool pushSuccess = buffer.push(std::vector<uint8_t>{7, 8, 9});
    ASSERT(!pushSuccess, "Push should fail after close");
    
    // Can still pop existing frames
    std::vector<uint8_t> frame;
    ASSERT(buffer.pop(frame, 100), "Can pop existing frames");
    ASSERT(buffer.pop(frame, 100), "Can pop existing frames");
    
    // Pop on empty closed buffer fails
    ASSERT(!buffer.pop(frame, 100), "Pop on empty closed buffer should fail");
}

// ==================== Main Test Runner ====================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "RTP Pipeline Test Suite\n";
    std::cout << "========================================\n";
    
    // Run all tests
    test_FrameBuffer_PushPop_wrapper();
    test_FrameBuffer_BlockingBehavior_wrapper();
    test_FrameBuffer_Timeout_wrapper();
    test_FrameBuffer_Close_wrapper();
    
    test_RTPPacket_EncodeDecode_wrapper();
    
    test_FrameReassembler_SinglePacket_wrapper();
    test_FrameReassembler_MultiPacket_InOrder_wrapper();
    test_FrameReassembler_MultipleFrames_wrapper();
    test_FrameReassembler_Cleanup_wrapper();
    
    // Summary
    std::cout << "\n";
    std::cout << "========================================\n";
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