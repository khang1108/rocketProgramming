/**
 * @file test_encoding_strategy.cpp
 * @brief Cross-platform test for EncodingStrategy (Linux/Windows)
 *
 * @details
 * Tests SDEncodingStrategy and HDEncodingStrategy on both platforms.
 * Compile on:
 * - Linux: g++ -std=c++17 -I../include -I../../common/include
 * test_encoding_strategy.cpp ../src/rtp/EncodingStrategy.cpp -o test_encoding
 * - Windows (MinGW): g++ -std=c++17 -I../include -I../../common/include
 * test_encoding_strategy.cpp ../src/rtp/EncodingStrategy.cpp -o
 * test_encoding.exe
 * - Windows (MSVC): cl /std:c++17 /I../include /I../../common/include
 * test_encoding_strategy.cpp ../src/rtp/EncodingStrategy.cpp
 */

<<<<<<< HEAD
#include "rtp/EncodingStrategy.hpp"
#include <cassert>
#include <iostream>
#include <vector>
=======
#include <cassert>
#include <iostream>
#include <vector>
#include "rtp/EncodingStrategy.hpp"
>>>>>>> origin/main

// Platform detection for display
#ifdef _WIN32
#define PLATFORM_NAME "Windows"
#ifdef _WIN64
#define ARCH_NAME "x64"
#else
#define ARCH_NAME "x86"
#endif
#elif __linux__
#define PLATFORM_NAME "Linux"
#ifdef __x86_64__
#define ARCH_NAME "x64"
#else
#define ARCH_NAME "x86"
#endif
#elif __APPLE__
#define PLATFORM_NAME "macOS"
#define ARCH_NAME "Universal"
#else
#define PLATFORM_NAME "Unknown"
#define ARCH_NAME "Unknown"
#endif

void printPlatformInfo() {
    std::cout << "========================================" << std::endl;
    std::cout << "  EncodingStrategy Cross-Platform Test" << std::endl;
    std::cout << "========================================" << std::endl;
<<<<<<< HEAD
    std::cout << "Platform: " << PLATFORM_NAME << " (" << ARCH_NAME << ")"
                << std::endl;
=======
    std::cout << "Platform: " << PLATFORM_NAME << " (" << ARCH_NAME << ")" << std::endl;
>>>>>>> origin/main
    std::cout << "C++ Version: " << __cplusplus << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void testSDEncoding() {
    std::cout << "Test 1: SD Encoding (Single Packet)" << std::endl;
    std::cout << "-----------------------------------" << std::endl;

    // Create small frame (< MAX_PAYLOAD_SIZE = 1400 bytes)
    std::vector<uint8_t> smallData(800, 0xFF);
    Frame smallFrame(smallData, 100, 90000, 12345678);

    SDEncodingStrategy sdStrategy;
    std::vector<RTPPacket> packets = sdStrategy.execute(smallFrame);

    // Verify
    assert(packets.size() == 1);
    std::cout << "✓ Frame size: " << smallData.size() << " bytes" << std::endl;
    std::cout << "✓ Packets generated: " << packets.size() << std::endl;
<<<<<<< HEAD
    std::cout << "✓ Marker bit: " << (packets[0].getMarker() ? "1 (last)" : "0")
                << std::endl;
    std::cout << "✓ Sequence number: " << packets[0].getSequenceNumber()
                << std::endl;
=======
    std::cout << "✓ Marker bit: " << (packets[0].getMarker() ? "1 (last)" : "0") << std::endl;
    std::cout << "✓ Sequence number: " << packets[0].getSequenceNumber() << std::endl;
>>>>>>> origin/main
    std::cout << "✓ Timestamp: " << packets[0].getTimestamp() << std::endl;
    std::cout << "✓ PASS: SD Encoding works!\n" << std::endl;
}

void testHDEncoding() {
    std::cout << "Test 2: HD Encoding (Fragmentation)" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    // Create large frame (> MAX_PAYLOAD_SIZE = 1400 bytes)
<<<<<<< HEAD
    const size_t largeSize = 5000; // 5KB frame
=======
    const size_t largeSize = 5000;  // 5KB frame
>>>>>>> origin/main
    std::vector<uint8_t> largeData(largeSize, 0xAA);
    Frame largeFrame(largeData, 200, 180000, 87654321);

    HDEncodingStrategy hdStrategy;
    std::vector<RTPPacket> packets = hdStrategy.execute(largeFrame);

    // Calculate expected packet count
<<<<<<< HEAD
    size_t expectedPackets = (largeSize + RTPPacket::MAX_PAYLOAD_SIZE - 1) /
                            RTPPacket::MAX_PAYLOAD_SIZE;
=======
    size_t expectedPackets =
        (largeSize + RTPPacket::MAX_PAYLOAD_SIZE - 1) / RTPPacket::MAX_PAYLOAD_SIZE;
>>>>>>> origin/main

    // Verify
    assert(packets.size() == expectedPackets);
    std::cout << "✓ Frame size: " << largeSize << " bytes" << std::endl;
    std::cout << "✓ Packets generated: " << packets.size() << std::endl;
    std::cout << "✓ Expected packets: " << expectedPackets << std::endl;

    // Check marker bits
    for (size_t i = 0; i < packets.size(); i++) {
        bool isLast = (i == packets.size() - 1);
        assert(packets[i].getMarker() == (isLast ? 1 : 0));
        std::cout << "✓ Packet " << i << ": marker=" << packets[i].getMarker()
<<<<<<< HEAD
                << ", seq=" << packets[i].getSequenceNumber()
                << ", ts=" << packets[i].getTimestamp() << std::endl;
=======
                  << ", seq=" << packets[i].getSequenceNumber()
                  << ", ts=" << packets[i].getTimestamp() << std::endl;
>>>>>>> origin/main
    }

    // Verify all timestamps are the same
    uint32_t firstTimestamp = packets[0].getTimestamp();
<<<<<<< HEAD
    for (const auto &packet : packets) {
        assert(packet.getTimestamp() == firstTimestamp);
    }
    std::cout << "✓ All fragments have same timestamp: " << firstTimestamp
                << std::endl;

    // Verify sequence numbers increment
    for (size_t i = 1; i < packets.size(); i++) {
        assert(packets[i].getSequenceNumber() ==
            packets[i - 1].getSequenceNumber() + 1);
=======
    for (const auto& packet : packets) {
        assert(packet.getTimestamp() == firstTimestamp);
    }
    std::cout << "✓ All fragments have same timestamp: " << firstTimestamp << std::endl;

    // Verify sequence numbers increment
    for (size_t i = 1; i < packets.size(); i++) {
        assert(packets[i].getSequenceNumber() == packets[i - 1].getSequenceNumber() + 1);
>>>>>>> origin/main
    }
    std::cout << "✓ Sequence numbers increment correctly" << std::endl;
    std::cout << "✓ PASS: HD Encoding works!\n" << std::endl;
}

void testEncodingContext() {
    std::cout << "Test 3: EncodingContext (Auto-detection)" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;

    EncodingContext context;

    // Test SD frame
    std::vector<uint8_t> sdData(800, 0xBB);
    Frame sdFrame(sdData, 300, 270000, 11111111);
    std::string info = context.getEncodingInfo(sdFrame);
    std::cout << "✓ SD Frame: " << info << std::endl;

    auto sdPackets = context.encodeFrame(sdFrame);
    assert(sdPackets.size() == 1);
    std::cout << "✓ SD packets: " << sdPackets.size() << std::endl;

    // Test HD frame
    std::vector<uint8_t> hdData(10000, 0xCC);
    Frame hdFrame(hdData, 400, 360000, 22222222);
    info = context.getEncodingInfo(hdFrame);
    std::cout << "✓ HD Frame: " << info << std::endl;

    auto hdPackets = context.encodeFrame(hdFrame);
    size_t expectedHdPackets =
        (10000 + RTPPacket::MAX_PAYLOAD_SIZE - 1) / RTPPacket::MAX_PAYLOAD_SIZE;
    assert(hdPackets.size() == expectedHdPackets);
    std::cout << "✓ HD packets: " << hdPackets.size() << std::endl;
    std::cout << "✓ PASS: EncodingContext auto-detection works!\n" << std::endl;
}

void testBoundaryConditions() {
    std::cout << "Test 4: Boundary Conditions" << std::endl;
    std::cout << "----------------------------" << std::endl;

    EncodingContext context;

    // Exactly MAX_PAYLOAD_SIZE
    std::vector<uint8_t> exactData(RTPPacket::MAX_PAYLOAD_SIZE, 0xDD);
    Frame exactFrame(exactData, 500, 450000, 33333333);
    auto exactPackets = context.encodeFrame(exactFrame);
    assert(exactPackets.size() == 1);
<<<<<<< HEAD
    std::cout << "✓ Exact MAX_PAYLOAD_SIZE: " << exactPackets.size() << " packet"
                << std::endl;
=======
    std::cout << "✓ Exact MAX_PAYLOAD_SIZE: " << exactPackets.size() << " packet" << std::endl;
>>>>>>> origin/main

    // MAX_PAYLOAD_SIZE + 1
    std::vector<uint8_t> overData(RTPPacket::MAX_PAYLOAD_SIZE + 1, 0xEE);
    Frame overFrame(overData, 600, 540000, 44444444);
    auto overPackets = context.encodeFrame(overFrame);
    assert(overPackets.size() == 2);
<<<<<<< HEAD
    std::cout << "✓ MAX_PAYLOAD_SIZE + 1: " << overPackets.size() << " packets"
                << std::endl;
=======
    std::cout << "✓ MAX_PAYLOAD_SIZE + 1: " << overPackets.size() << " packets" << std::endl;
>>>>>>> origin/main

    // Very small frame
    std::vector<uint8_t> tinyData(10, 0xFF);
    Frame tinyFrame(tinyData, 700, 630000, 55555555);
    auto tinyPackets = context.encodeFrame(tinyFrame);
    assert(tinyPackets.size() == 1);
<<<<<<< HEAD
    std::cout << "✓ Tiny frame (10 bytes): " << tinyPackets.size() << " packet"
                << std::endl;
=======
    std::cout << "✓ Tiny frame (10 bytes): " << tinyPackets.size() << " packet" << std::endl;
>>>>>>> origin/main

    std::cout << "✓ PASS: Boundary conditions handled correctly!\n" << std::endl;
}

int main() {
    try {
        printPlatformInfo();

        testSDEncoding();
        testHDEncoding();
        testEncodingContext();
        testBoundaryConditions();

        std::cout << "========================================" << std::endl;
        std::cout << "  ✅ ALL TESTS PASSED!" << std::endl;
        std::cout << "  Platform: " << PLATFORM_NAME << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
<<<<<<< HEAD
    } catch (const std::exception &e) {
=======
    } catch (const std::exception& e) {
>>>>>>> origin/main
        std::cerr << "\n❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
