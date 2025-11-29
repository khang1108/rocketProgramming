#ifndef COMMON_UTILS_TIMER_HPP
#define COMMON_UTILS_TIMER_HPP

#include <chrono>
#include<thread>

/**
 * @class Timer
 * @brief High-precision timer for frame rate control
 *
 * Used to maintain constant frame rate (e.g., 25 fps = 40ms interval)
 * Handles drift correction when processing takes longer than expected
 */
class Timer {
  private:
    int intervalMs_;  ///< Target interval between frames in milliseconds (e.g., 25 fps = 40ms)
    std::chrono::steady_clock::time_point startTime_;     ///< Start time of the current frame
    std::chrono::steady_clock::time_point lastWaitTime_;  ///< last wait() call time

  public:
    /**
     * @brief Constructor
     * @param intervalMs Target interval in milliseconds
     *
     * @example
     * Timer timer(40);  // 25 fps (1000ms / 25 = 40ms)
     */
    explicit Timer(int intervalMs);

    /**
     * @brief Start timer (reset interval)
     *
     * @details
     * Records current time as interval start
     * Call before processing work
     */
    void start();

    /**
     * @brief Wait until interval elapsed
     *
     * @details
     * Algorithm:
     * 1. Calculate elapsed time since start()
     * 2. If elapsed < interval, sleep for remaining time
     * 3. If elapsed >= interval, return immediately (drift correction)
     * 4. Reset start time for next interval
     *
     * @example
     * timer.start();
     * // ... process frame (takes 10ms) ...
     * timer.wait();  // Sleeps for 30ms to reach 40ms total
     */
    void wait();

    /**
     * @brief Get elapsed time since start()
     * @return Elapsed time in milliseconds
     */
    double getElapsed() const;

    /**
     * @brief Set new interval
     * @param intervalMs New interval in milliseconds
     */
    void setInterval(int intervalMs);

    /**
     * @brief Get current interval
     * @return Interval in milliseconds
     */
    int getInterval() const { return intervalMs_; }
};

#endif