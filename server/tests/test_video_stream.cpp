/**
 * @file test_video_stream.cpp
 * @brief Comprehensive test suite for VideoStream class
 *
 * Test Categories:
 * 1. Constructor Tests (file opening, validation)
 * 2. Frame Reading Tests (nextFrame(), hasMoreFrames())
 * 3. Navigation Tests (rewind(), getCurrentFrameNumber())
 * 4. Error Handling Tests (invalid files, corrupt data)
 * 5. Boundary Tests (empty file, single frame, large frames)
 *
 * Build & Run:
 *   Linux:   g++ -std=c++17 -I../../server/include -I../../common/include \
 *            test_video_stream.cpp ../src/video/VideoStream.cpp -o test_video_stream &&
 * ./test_video_stream
 *
 *   Windows: cl /EHsc /std:c++17 /I..\..\server\include /I..\..\common\include \
 *            test_video_stream.cpp ..\src\video\VideoStream.cpp /Fe:test_video_stream.exe
 *
 * Or use Makefile:
 *   make video
 *   make run_video
 */

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "video/VideoStream.hpp"

// ============================================================================
// Test Reporter Class
// ============================================================================

class TestReporter {
  private:
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    std::string currentCategory;

  public:
    void startCategory(const std::string& category) {
        currentCategory = category;
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "📋 " << category << "\n";
        std::cout << std::string(70, '=') << "\n";
    }

    void test(const std::string& testName, bool passed, const std::string& details = "") {
        totalTests++;
        if (passed) {
            passedTests++;
            std::cout << "✅ PASS: " << testName << "\n";
        } else {
            failedTests++;
            std::cout << "❌ FAIL: " << testName << "\n";
            if (!details.empty()) {
                std::cout << "   Details: " << details << "\n";
            }
        }
    }

    void printSummary() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "📊 TEST SUMMARY\n";
        std::cout << std::string(70, '=') << "\n";
        std::cout << "Total Tests:  " << totalTests << "\n";
        std::cout << "✅ Passed:     " << passedTests << "\n";
        std::cout << "❌ Failed:     " << failedTests << "\n";
        std::cout << "Success Rate: " << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0)
                  << "%\n";
        std::cout << std::string(70, '=') << "\n";
    }

    bool allPassed() const { return failedTests == 0; }
};

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Create a mock MJPEG file for testing
 * @param filename Output filename
 * @param numFrames Number of frames to create
 * @param frameSize Size of each frame in bytes
 * @return true if file created successfully
 *
 * MJPEG Format: [5-byte header][frame data][5-byte header][frame data]...
 * Header: Big-endian 40-bit integer (frame length)
 */
bool createMockMJPEGFile(const std::string& filename, int numFrames, size_t frameSize) {
    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }

    // Create dummy JPEG-like data (starts with JPEG magic bytes FF D8)
    std::vector<uint8_t> frameData(frameSize);
    frameData[0] = 0xFF;  // JPEG magic number
    frameData[1] = 0xD8;  // JPEG magic number
    frameData[2] = 0xFF;  // JPEG segment

    // Fill rest with pattern
    for (size_t i = 3; i < frameSize; i++) {
        frameData[i] = static_cast<uint8_t>(i % 256);
    }

    // Write each frame
    for (int f = 0; f < numFrames; f++) {
        // Write 5-byte header (frame length in big-endian)
        uint64_t length = frameSize;
        uint8_t header[5];
        header[0] = static_cast<uint8_t>((length >> 32) & 0xFF);
        header[1] = static_cast<uint8_t>((length >> 24) & 0xFF);
        header[2] = static_cast<uint8_t>((length >> 16) & 0xFF);
        header[3] = static_cast<uint8_t>((length >> 8) & 0xFF);
        header[4] = static_cast<uint8_t>(length & 0xFF);

        outFile.write(reinterpret_cast<char*>(header), 5);

        // Modify frame data slightly for each frame
        frameData[frameSize - 1] = static_cast<uint8_t>(f);

        // Write frame data
        outFile.write(reinterpret_cast<char*>(frameData.data()), frameSize);
    }

    outFile.close();
    return true;
}

/**
 * @brief Create an empty file
 */
bool createEmptyFile(const std::string& filename) {
    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    outFile.close();
    return true;
}

/**
 * @brief Create a corrupt MJPEG file (incomplete header)
 */
bool createCorruptMJPEGFile(const std::string& filename) {
    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }

    // Write only 3 bytes instead of 5-byte header
    uint8_t header[3] = {0x00, 0x00, 0x10};
    outFile.write(reinterpret_cast<char*>(header), 3);
    outFile.close();
    return true;
}

/**
 * @brief Delete a file
 */
void deleteFile(const std::string& filename) {
    std::remove(filename.c_str());
}

// ============================================================================
// Test Functions
// ============================================================================

void testConstructor(TestReporter& reporter) {
    reporter.startCategory("CONSTRUCTOR TESTS");

    // Test 1: Valid file
    {
        const std::string filename = "test_valid.mjpeg";
        createMockMJPEGFile(filename, 3, 1000);

        try {
            VideoStream video(filename);
            reporter.test("Constructor with valid file", true);
        } catch (const std::exception& e) {
            reporter.test("Constructor with valid file", false, e.what());
        }

        deleteFile(filename);
    }

    // Test 2: Non-existent file
    {
        try {
            VideoStream video("nonexistent_file.mjpeg");
            reporter.test("Constructor with non-existent file (should throw)", false,
                          "Expected exception but none thrown");
        } catch (const std::runtime_error& e) {
            bool correctError = std::string(e.what()).find("Khong the mo") != std::string::npos ||
                                std::string(e.what()).find("ERROR") != std::string::npos;
            reporter.test("Constructor with non-existent file throws exception", correctError);
        }
    }

    // Test 3: Empty file
    {
        const std::string filename = "test_empty.mjpeg";
        createEmptyFile(filename);

        try {
            VideoStream video(filename);
            reporter.test("Constructor with empty file (should throw)", false,
                          "Expected exception but none thrown");
        } catch (const std::runtime_error& e) {
            bool correctError = std::string(e.what()).find("rong") != std::string::npos ||
                                std::string(e.what()).find("empty") != std::string::npos;
            reporter.test("Constructor with empty file throws exception", correctError);
        }

        deleteFile(filename);
    }
}

void testFrameReading(TestReporter& reporter) {
    reporter.startCategory("FRAME READING TESTS");

    const std::string filename = "test_reading.mjpeg";
    const int numFrames = 5;
    const size_t frameSize = 1500;

    createMockMJPEGFile(filename, numFrames, frameSize);

    try {
        VideoStream video(filename);

        // Test 1: Read all frames
        int framesRead = 0;
        while (video.hasMoreFrames()) {
            auto frame = video.nextFrame();
            framesRead++;

            // Verify frame size
            if (frame.size() != frameSize) {
                reporter.test("Frame " + std::to_string(framesRead) + " size", false,
                              "Expected " + std::to_string(frameSize) + " but got " +
                                  std::to_string(frame.size()));
                break;
            }

            // Verify JPEG magic bytes
            if (frame[0] != 0xFF || frame[1] != 0xD8) {
                reporter.test("Frame " + std::to_string(framesRead) + " JPEG magic", false,
                              "Invalid JPEG header");
                break;
            }
        }

        reporter.test(
            "Read all frames correctly", framesRead == numFrames,
            "Expected " + std::to_string(numFrames) + " frames, got " + std::to_string(framesRead));

        // Test 2: hasMoreFrames() returns false after reading all
        reporter.test("hasMoreFrames() is false at end", !video.hasMoreFrames());

        // Test 3: Trying to read beyond EOF should throw
        try {
            video.nextFrame();
            reporter.test("nextFrame() at EOF throws exception", false,
                          "Expected exception but none thrown");
        } catch (const std::runtime_error& e) {
            reporter.test("nextFrame() at EOF throws exception", true);
        }

    } catch (const std::exception& e) {
        reporter.test("Frame reading setup", false, e.what());
    }

    deleteFile(filename);
}

void testNavigation(TestReporter& reporter) {
    reporter.startCategory("NAVIGATION TESTS");

    const std::string filename = "test_navigation.mjpeg";
    const int numFrames = 4;
    const size_t frameSize = 800;

    createMockMJPEGFile(filename, numFrames, frameSize);

    try {
        VideoStream video(filename);

        // Test 1: Initial frame number is 0
        reporter.test("Initial frame number is 0", video.getCurrentFrameNumber() == 0);

        // Test 2: Frame number increments after reading
        video.nextFrame();
        reporter.test("Frame number increments to 1", video.getCurrentFrameNumber() == 1);

        video.nextFrame();
        reporter.test("Frame number increments to 2", video.getCurrentFrameNumber() == 2);

        // Test 3: Rewind resets frame number
        video.rewind();
        reporter.test("Rewind resets frame number to 0", video.getCurrentFrameNumber() == 0);

        // Test 4: hasMoreFrames() is true after rewind
        reporter.test("hasMoreFrames() is true after rewind", video.hasMoreFrames());

        // Test 5: Can read again after rewind
        int framesRead = 0;
        while (video.hasMoreFrames()) {
            video.nextFrame();
            framesRead++;
        }
        reporter.test("Can read all frames again after rewind", framesRead == numFrames);

        // Test 6: Multiple rewinds work
        video.rewind();
        video.rewind();
        reporter.test("Multiple rewinds work",
                      video.getCurrentFrameNumber() == 0 && video.hasMoreFrames());

    } catch (const std::exception& e) {
        reporter.test("Navigation test setup", false, e.what());
    }

    deleteFile(filename);
}

void testErrorHandling(TestReporter& reporter) {
    reporter.startCategory("ERROR HANDLING TESTS");

    // Test 1: Corrupt file (incomplete header)
    {
        const std::string filename = "test_corrupt.mjpeg";
        createCorruptMJPEGFile(filename);

        try {
            VideoStream video(filename);
            try {
                video.nextFrame();
                reporter.test("nextFrame() with corrupt header throws", false,
                              "Expected exception but none thrown");
            } catch (const std::runtime_error& e) {
                reporter.test("nextFrame() with corrupt header throws exception", true);
            }
        } catch (const std::exception& e) {
            reporter.test("Open corrupt file", true);
        }

        deleteFile(filename);
    }

    // Test 2: Frame with abnormally large size (>50MB)
    {
        const std::string filename = "test_large_header.mjpeg";
        std::ofstream outFile(filename, std::ios::out | std::ios::binary);

        // Write header claiming 60MB frame (should be rejected)
        uint64_t hugeLength = 60ULL * 1024 * 1024;
        uint8_t header[5];
        header[0] = static_cast<uint8_t>((hugeLength >> 32) & 0xFF);
        header[1] = static_cast<uint8_t>((hugeLength >> 24) & 0xFF);
        header[2] = static_cast<uint8_t>((hugeLength >> 16) & 0xFF);
        header[3] = static_cast<uint8_t>((hugeLength >> 8) & 0xFF);
        header[4] = static_cast<uint8_t>(hugeLength & 0xFF);
        outFile.write(reinterpret_cast<char*>(header), 5);
        outFile.close();

        try {
            VideoStream video(filename);
            try {
                video.nextFrame();
                reporter.test("nextFrame() with huge frame size throws", false,
                              "Expected exception but none thrown");
            } catch (const std::runtime_error& e) {
                bool correctError = std::string(e.what()).find("qua lon") != std::string::npos ||
                                    std::string(e.what()).find("thuong") != std::string::npos;
                reporter.test("nextFrame() with huge frame size throws exception", correctError);
            }
        } catch (const std::exception& e) {
            reporter.test("Open file with huge frame", true);
        }

        deleteFile(filename);
    }

    // Test 3: Incomplete frame data (header says 1000 bytes but file ends early)
    {
        const std::string filename = "test_incomplete.mjpeg";
        std::ofstream outFile(filename, std::ios::out | std::ios::binary);

        // Write header claiming 1000 bytes
        uint64_t length = 1000;
        uint8_t header[5];
        header[0] = static_cast<uint8_t>((length >> 32) & 0xFF);
        header[1] = static_cast<uint8_t>((length >> 24) & 0xFF);
        header[2] = static_cast<uint8_t>((length >> 16) & 0xFF);
        header[3] = static_cast<uint8_t>((length >> 8) & 0xFF);
        header[4] = static_cast<uint8_t>(length & 0xFF);
        outFile.write(reinterpret_cast<char*>(header), 5);

        // But only write 100 bytes of data
        std::vector<uint8_t> data(100, 0xAA);
        outFile.write(reinterpret_cast<char*>(data.data()), 100);
        outFile.close();

        try {
            VideoStream video(filename);
            try {
                video.nextFrame();
                reporter.test("nextFrame() with incomplete frame data throws", false,
                              "Expected exception but none thrown");
            } catch (const std::runtime_error& e) {
                bool correctError = std::string(e.what()).find("dot ngot") != std::string::npos ||
                                    std::string(e.what()).find("ket thuc") != std::string::npos;
                reporter.test("nextFrame() with incomplete frame throws exception", correctError);
            }
        } catch (const std::exception& e) {
            reporter.test("Open incomplete file", true);
        }

        deleteFile(filename);
    }
}

void testBoundaryConditions(TestReporter& reporter) {
    reporter.startCategory("BOUNDARY CONDITION TESTS");

    // Test 1: Single frame file
    {
        const std::string filename = "test_single_frame.mjpeg";
        createMockMJPEGFile(filename, 1, 500);

        try {
            VideoStream video(filename);

            reporter.test("Single frame: hasMoreFrames() is true initially", video.hasMoreFrames());

            auto frame = video.nextFrame();
            reporter.test("Single frame: can read frame", frame.size() == 500);

            reporter.test("Single frame: hasMoreFrames() false after reading",
                          !video.hasMoreFrames());

            video.rewind();
            reporter.test("Single frame: can rewind and read again", video.hasMoreFrames());

        } catch (const std::exception& e) {
            reporter.test("Single frame test", false, e.what());
        }

        deleteFile(filename);
    }

    // Test 2: Very small frame (10 bytes)
    {
        const std::string filename = "test_tiny_frame.mjpeg";
        createMockMJPEGFile(filename, 2, 10);

        try {
            VideoStream video(filename);
            auto frame = video.nextFrame();
            reporter.test("Very small frame (10 bytes)", frame.size() == 10);
        } catch (const std::exception& e) {
            reporter.test("Very small frame", false, e.what());
        }

        deleteFile(filename);
    }

    // Test 3: Large frame (1MB - realistic for high quality JPEG)
    {
        const std::string filename = "test_large_frame.mjpeg";
        const size_t largeSize = 1024 * 1024;  // 1MB
        createMockMJPEGFile(filename, 1, largeSize);

        try {
            VideoStream video(filename);
            auto frame = video.nextFrame();
            reporter.test("Large frame (1MB)", frame.size() == largeSize);
        } catch (const std::exception& e) {
            reporter.test("Large frame", false, e.what());
        }

        deleteFile(filename);
    }

    // Test 4: Many frames (100 frames)
    {
        const std::string filename = "test_many_frames.mjpeg";
        const int manyFrames = 100;
        createMockMJPEGFile(filename, manyFrames, 500);

        try {
            VideoStream video(filename);
            int count = 0;
            while (video.hasMoreFrames()) {
                video.nextFrame();
                count++;
            }
            reporter.test("Read 100 frames successfully", count == manyFrames);
        } catch (const std::exception& e) {
            reporter.test("Many frames test", false, e.what());
        }

        deleteFile(filename);
    }

    // Test 5: Variable frame sizes
    {
        const std::string filename = "test_variable_sizes.mjpeg";
        std::ofstream outFile(filename, std::ios::out | std::ios::binary);

        std::vector<size_t> frameSizes = {100, 500, 1500, 800, 2000};

        for (size_t size : frameSizes) {
            // Write header
            uint8_t header[5];
            header[0] = static_cast<uint8_t>((size >> 32) & 0xFF);
            header[1] = static_cast<uint8_t>((size >> 24) & 0xFF);
            header[2] = static_cast<uint8_t>((size >> 16) & 0xFF);
            header[3] = static_cast<uint8_t>((size >> 8) & 0xFF);
            header[4] = static_cast<uint8_t>(size & 0xFF);
            outFile.write(reinterpret_cast<char*>(header), 5);

            // Write frame data
            std::vector<uint8_t> data(size, 0xBB);
            data[0] = 0xFF;
            data[1] = 0xD8;
            outFile.write(reinterpret_cast<char*>(data.data()), size);
        }
        outFile.close();

        try {
            VideoStream video(filename);
            bool allCorrect = true;
            for (size_t expectedSize : frameSizes) {
                auto frame = video.nextFrame();
                if (frame.size() != expectedSize) {
                    allCorrect = false;
                    break;
                }
            }
            reporter.test("Variable frame sizes read correctly", allCorrect);
        } catch (const std::exception& e) {
            reporter.test("Variable frame sizes", false, e.what());
        }

        deleteFile(filename);
    }
}

void testLoopPlayback(TestReporter& reporter) {
    reporter.startCategory("LOOP PLAYBACK SIMULATION");

    const std::string filename = "test_loop.mjpeg";
    const int numFrames = 3;
    createMockMJPEGFile(filename, numFrames, 600);

    try {
        VideoStream video(filename);

        // Simulate playing video 3 times in a loop
        int totalFramesRead = 0;
        for (int loop = 0; loop < 3; loop++) {
            int framesInThisLoop = 0;
            while (video.hasMoreFrames()) {
                video.nextFrame();
                framesInThisLoop++;
                totalFramesRead++;
            }

            if (loop < 2) {  // Don't rewind after last loop
                video.rewind();
            }
        }

        reporter.test("Loop playback (3 loops)", totalFramesRead == numFrames * 3,
                      "Expected " + std::to_string(numFrames * 3) + " frames, got " +
                          std::to_string(totalFramesRead));

    } catch (const std::exception& e) {
        reporter.test("Loop playback", false, e.what());
    }

    deleteFile(filename);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════╗
║                 VideoStream Class Test Suite                        ║
║                  MJPEG File Reader Testing                          ║
╚══════════════════════════════════════════════════════════════════════╝
)" << std::endl;

    TestReporter reporter;

    // Run all test categories
    testConstructor(reporter);
    testFrameReading(reporter);
    testNavigation(reporter);
    testErrorHandling(reporter);
    testBoundaryConditions(reporter);
    testLoopPlayback(reporter);

    // Print summary
    reporter.printSummary();

    // Return appropriate exit code
    return reporter.allPassed() ? 0 : 1;
}
