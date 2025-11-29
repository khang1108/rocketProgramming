/**
 * @file test_rtp_packet.cpp
 * @brief Comprehensive unit tests for RTPPacket and RTPPacketBuilder
 *
 * @details
 * Tests cover:
 * - RTPPacket encoding/decoding
 * - RTPPacketBuilder fluent interface
 * - Edge cases and error handling
 * - Cross-platform compatibility
 * - SD and HD fragmentation scenarios
 *
 * Compile:
 * g++ -std=c++17 -I../include -I../../common/include \
 *     test_rtp_packet.cpp ../src/rtp/RTPPacket.cpp \
 *     -o test_rtp_packet
 */

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>
#include "rtp/RTPPacket.hpp"
#include "rtp/RTPPacketBuilder.hpp"

// Test utilities
class TestReporter {
  private:
    int totalTests = 0;
    int totalAssertions = 0;
    std::string currentTest;

  public:
    void startTest(const std::string& name) {
        currentTest = name;
        totalTests++;
        std::cout << "\n[TEST] " << name << std::endl;
    }

    void pass(const std::string& msg = "") {
        totalAssertions++;
        std::cout << "  ✓ PASS";
        if (!msg.empty())
            std::cout << ": " << msg;
        std::cout << std::endl;
    }

    void fail(const std::string& msg) { std::cout << "  ✗ FAIL: " << msg << std::endl; }

    void summary() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Summary:" << std::endl;
        std::cout << "  Total Test Functions: " << totalTests << std::endl;
        std::cout << "  Total Assertions: " << totalAssertions << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "✓ ALL TESTS PASSED!" << std::endl;
    }
};

TestReporter reporter;

// Helper function to print bytes in hex
void printBytes(const uint8_t* data, size_t len, const std::string& label) {
    std::cout << "  " << label << ": ";
    for (size_t i = 0; i < len && i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i])
                  << " ";
    }
    if (len > 32)
        std::cout << "...";
    std::cout << std::dec << std::endl;
}

// ==================== TEST CASES ====================

void testRTPPacketBasicEncoding() {
    reporter.startTest("RTPPacket Basic Encoding");

    RTPPacket packet;
    packet.setVersion(2);
    packet.setPadding(0);
    packet.setExtension(0);
    packet.setCC(0);
    packet.setMarker(1);
    packet.setPayloadType(26);
    packet.setSequenceNumber(100);
    packet.setTimestamp(12345);
    packet.setSSRC(99999);

    std::vector<uint8_t> payload = {0xAA, 0xBB, 0xCC, 0xDD};
    packet.setPayload(payload);

    packet.encode();

    // Verify header bytes
    const auto& header = packet.getHeader();

    // Byte 0: V=2, P=0, X=0, CC=0 → 10000000 = 0x80
    assert(header[0] == 0x80);
    reporter.pass("Version, Padding, Extension, CC encoded correctly");

    // Byte 1: M=1, PT=26 → 10011010 = 0x9A
    assert(header[1] == 0x9A);
    reporter.pass("Marker and Payload Type encoded correctly");

    // Bytes 2-3: Sequence = 100 → 0x0064
    assert(header[2] == 0x00 && header[3] == 0x64);
    reporter.pass("Sequence number encoded correctly");

    // Bytes 4-7: Timestamp = 12345 → 0x00003039
    assert(header[4] == 0x00 && header[5] == 0x00 && header[6] == 0x30 && header[7] == 0x39);
    reporter.pass("Timestamp encoded correctly");

    // Bytes 8-11: SSRC = 99999 → 0x0001869F
    assert(header[8] == 0x00 && header[9] == 0x01 && header[10] == 0x86 && header[11] == 0x9F);
    reporter.pass("SSRC encoded correctly");

    assert(packet.getPayloadSize() == 4);
    reporter.pass("Payload size correct");
}

void testRTPPacketDecoding() {
    reporter.startTest("RTPPacket Decoding");

    // Create a raw packet
    uint8_t rawPacket[] = {
        0x80,                    // V=2, P=0, X=0, CC=0
        0x9A,                    // M=1, PT=26
        0x00, 0x64,              // Seq = 100
        0x00, 0x00, 0x30, 0x39,  // TS = 12345
        0x00, 0x01, 0x86, 0x9F,  // SSRC = 99999
        0xAA, 0xBB, 0xCC, 0xDD   // Payload
    };

    RTPPacket packet(rawPacket, sizeof(rawPacket));

    assert(packet.getVersion() == 2);
    reporter.pass("Version decoded correctly");

    assert(packet.getMarker() == 1);
    reporter.pass("Marker decoded correctly");

    assert(packet.getPayloadType() == 26);
    reporter.pass("Payload type decoded correctly");

    assert(packet.getSequenceNumber() == 100);
    reporter.pass("Sequence number decoded correctly");

    assert(packet.getTimestamp() == 12345);
    reporter.pass("Timestamp decoded correctly");

    assert(packet.getSSRC() == 99999);
    reporter.pass("SSRC decoded correctly");

    const auto& payload = packet.getPayload();
    assert(payload.size() == 4);
    assert(payload[0] == 0xAA && payload[1] == 0xBB && payload[2] == 0xCC && payload[3] == 0xDD);
    reporter.pass("Payload decoded correctly");
}

void testRTPPacketRoundTrip() {
    reporter.startTest("RTPPacket Round-Trip (Encode → Decode)");

    // Original packet
    RTPPacket original;
    original.setVersion(2);
    original.setMarker(1);
    original.setPayloadType(26);
    original.setSequenceNumber(555);
    original.setTimestamp(98765);
    original.setSSRC(12345678);

    std::vector<uint8_t> originalPayload(100, 0xFF);
    original.setPayload(originalPayload);
    original.encode();

    // Get packet bytes
    std::vector<uint8_t> packetBytes = original.getPacketVector();

    // Decode into new packet
    RTPPacket decoded(packetBytes.data(), packetBytes.size());

    // Verify all fields match
    assert(decoded.getVersion() == original.getVersion());
    assert(decoded.getMarker() == original.getMarker());
    assert(decoded.getPayloadType() == original.getPayloadType());
    assert(decoded.getSequenceNumber() == original.getSequenceNumber());
    assert(decoded.getTimestamp() == original.getTimestamp());
    assert(decoded.getSSRC() == original.getSSRC());
    assert(decoded.getPayloadSize() == original.getPayloadSize());

    reporter.pass("All fields preserved after encode/decode");
}

void testRTPPacketBuilderBasic() {
    reporter.startTest("RTPPacketBuilder Basic Usage");

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5};

    RTPPacket packet = RTPPacketBuilder()
                           .setPayloadType(RTPPacket::MJPEG_TYPE)
                           .setSequenceNumber(10)
                           .setTimestamp(1000)
                           .setSSRC(888)
                           .setMarker(0)
                           .setPayload(payload)
                           .build();

    assert(packet.getVersion() == 2);
    reporter.pass("Default version set");

    assert(packet.getPayloadType() == 26);
    reporter.pass("Payload type set");

    assert(packet.getSequenceNumber() == 10);
    reporter.pass("Sequence number set");

    assert(packet.getTimestamp() == 1000);
    reporter.pass("Timestamp set");

    assert(packet.getSSRC() == 888);
    reporter.pass("SSRC set");

    assert(packet.getMarker() == 0);
    reporter.pass("Marker set");

    assert(packet.getPayloadSize() == 5);
    reporter.pass("Payload set");
}

void testRTPPacketBuilderFluentInterface() {
    reporter.startTest("RTPPacketBuilder Fluent Interface (Method Chaining)");

    std::vector<uint8_t> data(50, 0xAB);

    RTPPacket packet = RTPPacketBuilder()
                           .setVersion(2)
                           .setPadding(0)
                           .setExtension(0)
                           .setCC(0)
                           .setMarker(1)
                           .setPayloadType(26)
                           .setSequenceNumber(999)
                           .setTimestamp(RTPPacket::getCurrentTimestamp())
                           .setSSRC(11111)
                           .setPayload(data)
                           .build();

    assert(packet.getSequenceNumber() == 999);
    assert(packet.getMarker() == 1);
    reporter.pass("Method chaining works correctly");
}

void testRTPPacketBuilderMissingFields() {
    reporter.startTest("RTPPacketBuilder Error: Missing Required Fields");

    bool caught = false;
    try {
        // Missing sequenceNumber
        RTPPacketBuilder builder;
        builder.setTimestamp(100).setSSRC(200).setPayload(std::vector<uint8_t>(10, 0xFF)).build();
    } catch (const std::runtime_error& e) {
        caught = true;
        reporter.pass("Exception thrown for missing sequence number");
    }
    assert(caught);

    caught = false;
    try {
        // Missing timestamp
        RTPPacketBuilder builder;
        builder.setSequenceNumber(100)
            .setSSRC(200)
            .setPayload(std::vector<uint8_t>(10, 0xFF))
            .build();
    } catch (const std::runtime_error& e) {
        caught = true;
        reporter.pass("Exception thrown for missing timestamp");
    }
    assert(caught);

    caught = false;
    try {
        // Missing SSRC
        RTPPacketBuilder builder;
        builder.setSequenceNumber(100)
            .setTimestamp(200)
            .setPayload(std::vector<uint8_t>(10, 0xFF))
            .build();
    } catch (const std::runtime_error& e) {
        caught = true;
        reporter.pass("Exception thrown for missing SSRC");
    }
    assert(caught);

    caught = false;
    try {
        // Missing payload
        RTPPacketBuilder builder;
        builder.setSequenceNumber(100).setTimestamp(200).setSSRC(300).build();
    } catch (const std::runtime_error& e) {
        caught = true;
        reporter.pass("Exception thrown for missing payload");
    }
    assert(caught);
}

void testRTPPacketBuilderInvalidData() {
    reporter.startTest("RTPPacketBuilder Error: Invalid Data");

    bool caught = false;
    try {
        RTPPacketBuilder builder;
        builder.setVersion(3);  // Invalid version
    } catch (const std::invalid_argument& e) {
        caught = true;
        reporter.pass("Exception thrown for invalid version");
    }
    assert(caught);

    caught = false;
    try {
        RTPPacketBuilder builder;
        builder.setCC(20);  // CC must be 0-15
    } catch (const std::invalid_argument& e) {
        caught = true;
        reporter.pass("Exception thrown for invalid CC");
    }
    assert(caught);

    caught = false;
    try {
        RTPPacketBuilder builder;
        builder.setPayloadType(200);  // PT must be 0-127
    } catch (const std::invalid_argument& e) {
        caught = true;
        reporter.pass("Exception thrown for invalid payload type");
    }
    assert(caught);

    caught = false;
    try {
        RTPPacketBuilder builder;
        builder.setPayload(nullptr, 10);  // Null payload
    } catch (const std::invalid_argument& e) {
        caught = true;
        reporter.pass("Exception thrown for null payload");
    }
    assert(caught);
}

void testRTPPacketBuilderReset() {
    reporter.startTest("RTPPacketBuilder Reset");

    RTPPacketBuilder builder;

    // Build first packet
    std::vector<uint8_t> payload1 = {1, 2, 3};
    RTPPacket packet1 =
        builder.setSequenceNumber(1).setTimestamp(100).setSSRC(999).setPayload(payload1).build();

    assert(packet1.getSequenceNumber() == 1);

    // Reset builder
    builder.reset();
    assert(!builder.isReady());
    reporter.pass("Builder reset successfully");

    // Build second packet
    std::vector<uint8_t> payload2 = {4, 5, 6};
    RTPPacket packet2 =
        builder.setSequenceNumber(2).setTimestamp(200).setSSRC(888).setPayload(payload2).build();

    assert(packet2.getSequenceNumber() == 2);
    reporter.pass("Builder reused after reset");
}

void testSDVideoScenario() {
    reporter.startTest("SD Video Scenario (Single Packet Per Frame)");

    // Simulate SD frame (< 1400 bytes)
    std::vector<uint8_t> frameData(800, 0xCD);

    RTPPacket packet = RTPPacketBuilder()
                           .setPayloadType(RTPPacket::MJPEG_TYPE)
                           .setSequenceNumber(1)
                           .setTimestamp(RTPPacket::getCurrentTimestamp())
                           .setSSRC(12345)
                           .setMarker(1)  // Single packet = complete frame
                           .setPayload(frameData)
                           .build();

    assert(packet.getMarker() == 1);
    assert(packet.getPayloadSize() == 800);
    reporter.pass("SD frame encoded as single packet");

    // Simulate sending
    std::vector<uint8_t> packetBytes = packet.getPacketVector();
    assert(packetBytes.size() == RTPPacket::HEADER_SIZE + 800);
    reporter.pass("Packet ready to send");
}

void testHDVideoFragmentation() {
    reporter.startTest("HD Video Fragmentation (Multiple Packets)");

    // Simulate HD frame (> 1400 bytes)
    std::vector<uint8_t> largeFrame(5000, 0xDE);
    size_t maxPayload = RTPPacket::MAX_PAYLOAD_SIZE;

    std::vector<RTPPacket> packets;
    size_t offset = 0;
    uint16_t seqNum = 100;
    uint32_t timestamp = RTPPacket::getCurrentTimestamp();

    while (offset < largeFrame.size()) {
        size_t chunkSize = std::min(maxPayload, largeFrame.size() - offset);
        bool isLast = (offset + chunkSize >= largeFrame.size());

        RTPPacket packet = RTPPacketBuilder()
                               .setPayloadType(RTPPacket::MJPEG_TYPE)
                               .setSequenceNumber(seqNum++)
                               .setTimestamp(timestamp)  // Same timestamp for all fragments
                               .setSSRC(54321)
                               .setMarker(isLast ? 1 : 0)  // Marker = 1 only for last
                               .setPayload(&largeFrame[offset], chunkSize)
                               .build();

        packets.push_back(packet);
        offset += chunkSize;
    }

    size_t expectedPackets = (5000 + maxPayload - 1) / maxPayload;
    assert(packets.size() == expectedPackets);
    reporter.pass("Frame fragmented into " + std::to_string(packets.size()) + " packets");

    // Verify all packets have same timestamp
    uint32_t firstTimestamp = packets[0].getTimestamp();
    for (const auto& pkt : packets) {
        assert(pkt.getTimestamp() == firstTimestamp);
    }
    reporter.pass("All fragments have same timestamp");

    // Verify sequence numbers increment
    for (size_t i = 1; i < packets.size(); i++) {
        assert(packets[i].getSequenceNumber() == packets[i - 1].getSequenceNumber() + 1);
    }
    reporter.pass("Sequence numbers increment correctly");

    // Verify only last packet has marker = 1
    for (size_t i = 0; i < packets.size() - 1; i++) {
        assert(packets[i].getMarker() == 0);
    }
    assert(packets.back().getMarker() == 1);
    reporter.pass("Only last fragment has marker bit set");
}

void testSequenceNumberWrapAround() {
    reporter.startTest("Sequence Number Wrap-Around");

    // Test wrap from 65535 to 0
    int32_t diff1 = RTPPacket::sequenceDifference(0, 65535);
    assert(diff1 == 1);
    reporter.pass("Wrap-around 65535→0 handled correctly (diff=1)");

    int32_t diff2 = RTPPacket::sequenceDifference(65535, 0);
    assert(diff2 == -1);
    reporter.pass("Reverse wrap 0→65535 handled correctly (diff=-1)");

    int32_t diff3 = RTPPacket::sequenceDifference(100, 50);
    assert(diff3 == 50);
    reporter.pass("Normal difference calculated correctly");
}

void testPacketValidation() {
    reporter.startTest("Packet Validation");

    RTPPacket validPacket;
    validPacket.setVersion(2);
    validPacket.setPayloadType(26);
    assert(validPacket.validate());
    reporter.pass("Valid packet passes validation");

    RTPPacket invalidVersion;
    invalidVersion.setVersion(1);  // Invalid version
    invalidVersion.setPayloadType(26);
    assert(!invalidVersion.validate());
    reporter.pass("Invalid version fails validation");

    RTPPacket invalidPayloadType;
    invalidPayloadType.setVersion(2);
    invalidPayloadType.setPayloadType(10);  // Not MJPEG
    assert(!invalidPayloadType.validate());
    reporter.pass("Invalid payload type fails validation");
}

void testLargePayload() {
    reporter.startTest("Large Payload Handling");

    // Test with maximum recommended size
    std::vector<uint8_t> maxPayload(RTPPacket::MAX_PAYLOAD_SIZE, 0xEF);

    RTPPacket packet = RTPPacketBuilder()
                           .setSequenceNumber(1)
                           .setTimestamp(100)
                           .setSSRC(999)
                           .setPayload(maxPayload)
                           .build();

    assert(packet.getPayloadSize() == RTPPacket::MAX_PAYLOAD_SIZE);
    reporter.pass("Maximum payload size handled correctly");

    // Test with oversized payload (should still work, but not recommended)
    std::vector<uint8_t> oversized(2000, 0xAB);
    RTPPacket bigPacket = RTPPacketBuilder()
                              .setSequenceNumber(2)
                              .setTimestamp(200)
                              .setSSRC(888)
                              .setPayload(oversized)
                              .build();

    assert(bigPacket.getPayloadSize() == 2000);
    reporter.pass("Oversized payload accepted (warning: may exceed MTU)");
}

void testEmptyPayload() {
    reporter.startTest("Empty Payload Handling");

    bool caught = false;
    try {
        std::vector<uint8_t> empty;
        RTPPacketBuilder builder;
        builder.setSequenceNumber(1)
            .setTimestamp(100)
            .setSSRC(999)
            .setPayload(empty)  // Empty payload
            .build();
    } catch (const std::invalid_argument& e) {
        caught = true;
        reporter.pass("Exception thrown for empty payload");
    }
    assert(caught);
}

void testGetPacketBuffer() {
    reporter.startTest("Get Packet Buffer (for sendto)");

    std::vector<uint8_t> payload = {0x11, 0x22, 0x33};
    RTPPacket packet = RTPPacketBuilder()
                           .setSequenceNumber(10)
                           .setTimestamp(1000)
                           .setSSRC(5000)
                           .setPayload(payload)
                           .build();

    uint8_t buffer[1500];
    size_t written = packet.getPacket(buffer, sizeof(buffer));

    assert(written == RTPPacket::HEADER_SIZE + 3);
    reporter.pass("Packet written to buffer correctly");

    // Verify header in buffer
    assert(buffer[0] == 0x80);  // Version = 2
    reporter.pass("Header in buffer is correct");

    // Verify payload in buffer
    assert(buffer[12] == 0x11 && buffer[13] == 0x22 && buffer[14] == 0x33);
    reporter.pass("Payload in buffer is correct");
}

void testTimestampGeneration() {
    reporter.startTest("Timestamp Generation");

    uint32_t ts1 = RTPPacket::getCurrentTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint32_t ts2 = RTPPacket::getCurrentTimestamp();

    assert(ts2 > ts1);
    reporter.pass("Timestamp increments over time");

    uint32_t diff = ts2 - ts1;
    assert(diff >= 10 && diff < 20);  // Should be ~10ms
    reporter.pass("Timestamp granularity is milliseconds");
}

// ==================== MAIN ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  RTPPacket & RTPPacketBuilder Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        // Basic RTPPacket tests
        testRTPPacketBasicEncoding();
        testRTPPacketDecoding();
        testRTPPacketRoundTrip();
        testPacketValidation();

        // RTPPacketBuilder tests
        testRTPPacketBuilderBasic();
        testRTPPacketBuilderFluentInterface();
        testRTPPacketBuilderReset();

        // Error handling tests
        testRTPPacketBuilderMissingFields();
        testRTPPacketBuilderInvalidData();
        testEmptyPayload();

        // Scenario tests
        testSDVideoScenario();
        testHDVideoFragmentation();
        testSequenceNumberWrapAround();

        // Utility tests
        testLargePayload();
        testGetPacketBuffer();
        testTimestampGeneration();

        reporter.summary();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ UNEXPECTED ERROR: " << e.what() << std::endl;
        return 1;
    }
}
