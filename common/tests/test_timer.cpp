/**
 * @file test_timer.cpp
 * @brief Test suite for Timer class
 */

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>
#include "../include/utils/Timer.hpp"

// Simple test framework
class TestReporter {
  private:
    int passed = 0;
    int failed = 0;
    std::string currentTest;

  public:
    void startTest(const std::string& name) {
        currentTest = name;
        std::cout << "Testing: " << name << " ... ";
    }

    void pass() {
        passed++;
        std::cout << "✓ PASS" << std::endl;
    }

    void fail(const std::string& message) {
        failed++;
        std::cout << "✗ FAIL: " << message << std::endl;
    }

    void report() {
        std::cout << "\n========================================\n";
        std::cout << "Total: " << (passed + failed) << " tests\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "========================================\n";
    }

    bool allPassed() const { return failed == 0; }
};

TestReporter reporter;

// Test helper
#define TEST(name) reporter.startTest(name)
#define ASSERT(condition, message) \
    if (!(condition)) {            \
        reporter.fail(message);    \
        return;                    \
    }
#define PASS() reporter.pass()

// Helper function to check if value is within tolerance
bool withinTolerance(double actual, double expected, double tolerance) {
    return std::abs(actual - expected) <= tolerance;
}

// ==================== Test Cases ====================

void test_constructor() {
    TEST("Constructor - Create timer with 40ms interval");

    Timer timer(40);

    ASSERT(timer.getInterval() == 40, "Interval not set correctly");

    PASS();
}

void test_getElapsed_immediate() {
    TEST("getElapsed - Immediately after start");

    Timer timer(40);
    timer.start();

    double elapsed = timer.getElapsed();

    // Should be very small (< 5ms)
    ASSERT(elapsed < 5.0,
           "Elapsed time too large immediately after start: " + std::to_string(elapsed) + "ms");

    PASS();
}

void test_getElapsed_afterDelay() {
    TEST("getElapsed - After 50ms sleep");

    Timer timer(100);
    timer.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    double elapsed = timer.getElapsed();

    // Should be around 50ms (tolerance ±10ms)
    ASSERT(withinTolerance(elapsed, 50.0, 10.0),
           "Elapsed time mismatch: expected ~50ms, got " + std::to_string(elapsed) + "ms");

    PASS();
}

void test_wait_basic() {
    TEST("wait - Basic timing (40ms interval)");

    Timer timer(40);
    timer.start();

    // Do some quick work (5ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto beforeWait = std::chrono::steady_clock::now();
    timer.wait();
    auto afterWait = std::chrono::steady_clock::now();

    auto totalElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(afterWait - beforeWait).count();

    // Should sleep for ~35ms (40ms - 5ms work)
    ASSERT(withinTolerance(totalElapsed, 35.0, 10.0),
           "Wait time mismatch: expected ~35ms, got " + std::to_string(totalElapsed) + "ms");

    PASS();
}

void test_wait_noSleepNeeded() {
    TEST("wait - No sleep needed (work took longer than interval)");

    Timer timer(40);
    timer.start();

    // Do work that takes longer than interval (50ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto beforeWait = std::chrono::steady_clock::now();
    timer.wait();
    auto afterWait = std::chrono::steady_clock::now();

    auto waitTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(afterWait - beforeWait).count();

    // Should return immediately (< 5ms)
    ASSERT(waitTime < 5, "Wait should return immediately when work exceeds interval, but took " +
                             std::to_string(waitTime) + "ms");

    PASS();
}

void test_wait_25fps() {
    TEST("wait - 25 FPS timing (40ms per frame)");

    Timer timer(40);  // 25 FPS = 40ms per frame

    auto start = std::chrono::steady_clock::now();

    // Simulate 10 frames
    for (int i = 0; i < 10; i++) {
        timer.start();

        // Simulate frame processing (5ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        timer.wait();
    }

    auto end = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 10 frames at 40ms each = 400ms (tolerance ±50ms)
    ASSERT(withinTolerance(totalTime, 400.0, 50.0),
           "Total time for 10 frames mismatch: expected ~400ms, got " + std::to_string(totalTime) +
               "ms");

    PASS();
}

void test_setInterval() {
    TEST("setInterval - Change interval dynamically");

    Timer timer(40);

    ASSERT(timer.getInterval() == 40, "Initial interval wrong");

    timer.setInterval(100);

    ASSERT(timer.getInterval() == 100, "Interval not updated");

    PASS();
}

void test_setInterval_invalid() {
    TEST("setInterval - Invalid interval (0 or negative)");

    Timer timer(40);

    timer.setInterval(0);
    ASSERT(timer.getInterval() == 40, "Interval should not change for 0");

    timer.setInterval(-10);
    ASSERT(timer.getInterval() == 40, "Interval should not change for negative value");

    PASS();
}

void test_multiple_starts() {
    TEST("Multiple start() calls - Timer reset");

    Timer timer(100);

    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    double elapsed1 = timer.getElapsed();
    ASSERT(withinTolerance(elapsed1, 50.0, 10.0),
           "First elapsed mismatch: " + std::to_string(elapsed1) + "ms");

    // Restart timer
    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    double elapsed2 = timer.getElapsed();
    ASSERT(withinTolerance(elapsed2, 30.0, 10.0),
           "Second elapsed after restart mismatch: " + std::to_string(elapsed2) + "ms");

    PASS();
}

void test_wait_precision() {
    TEST("wait - Precision test (5 consecutive frames)");

    Timer timer(50);  // 20 FPS

    std::vector<double> frameTimes;
    auto lastTime = std::chrono::steady_clock::now();

    for (int i = 0; i < 5; i++) {
        timer.start();

        // Simulate minimal work (1ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        timer.wait();

        auto now = std::chrono::steady_clock::now();
        double frameTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        frameTimes.push_back(frameTime);
        lastTime = now;
    }

    // Check all frame times are close to 50ms (tolerance ±15ms)
    bool allFramesGood = true;
    for (size_t i = 0; i < frameTimes.size(); i++) {
        if (!withinTolerance(frameTimes[i], 50.0, 15.0)) {
            allFramesGood = false;
            std::cout << "\n  Frame " << i << ": " << frameTimes[i] << "ms";
        }
    }

    ASSERT(allFramesGood, "Frame timing inconsistent");

    PASS();
}

void test_different_intervals() {
    TEST("Different intervals - 30fps, 60fps");

    // Test 30 FPS (33ms)
    Timer timer30(33);
    timer30.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto start30 = std::chrono::steady_clock::now();
    timer30.wait();
    auto end30 = std::chrono::steady_clock::now();
    auto wait30 = std::chrono::duration_cast<std::chrono::milliseconds>(end30 - start30).count();

    ASSERT(withinTolerance(wait30, 28.0, 10.0),  // 33 - 5 = 28ms
           "30fps wait time mismatch: " + std::to_string(wait30) + "ms");

    // Test 60 FPS (16ms)
    Timer timer60(16);
    timer60.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto start60 = std::chrono::steady_clock::now();
    timer60.wait();
    auto end60 = std::chrono::steady_clock::now();
    auto wait60 = std::chrono::duration_cast<std::chrono::milliseconds>(end60 - start60).count();

    ASSERT(withinTolerance(wait60, 11.0, 10.0),  // 16 - 5 = 11ms
           "60fps wait time mismatch: " + std::to_string(wait60) + "ms");

    PASS();
}

void test_stress_rapid_calls() {
    TEST("Stress test - Rapid wait() calls");

    Timer timer(10);  // Very short interval

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 50; i++) {
        timer.start();
        timer.wait();
    }

    auto end = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 50 frames at 10ms = 500ms (tolerance ±100ms)
    ASSERT(withinTolerance(totalTime, 500.0, 100.0),
           "Stress test timing off: expected ~500ms, got " + std::to_string(totalTime) + "ms");

    PASS();
}

void test_elapsed_without_start() {
    TEST("getElapsed - Without calling start()");

    Timer timer(40);

    // Constructor initializes startTime, so this should work
    double elapsed = timer.getElapsed();

    ASSERT(elapsed >= 0.0, "Elapsed should be non-negative");
    ASSERT(elapsed < 10.0, "Elapsed should be small without significant time passing");

    PASS();
}

// ==================== Main ====================

int main() {
    std::cout << "\n========================================\n";
    std::cout << "   Timer Test Suite\n";
    std::cout << "========================================\n\n";

    // Basic tests
    test_constructor();
    test_getElapsed_immediate();
    test_getElapsed_afterDelay();

    // Wait tests
    test_wait_basic();
    test_wait_noSleepNeeded();
    test_wait_25fps();
    test_wait_precision();

    // Interval tests
    test_setInterval();
    test_setInterval_invalid();
    test_different_intervals();

    // Advanced tests
    test_multiple_starts();
    test_stress_rapid_calls();
    test_elapsed_without_start();

    reporter.report();

    return reporter.allPassed() ? 0 : 1;
}
