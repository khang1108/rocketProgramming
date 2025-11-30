#ifndef CLIENT_UI_FRAMEDISPLAY_HPP
#define CLIENT_UI_FRAMEDISPLAY_HPP

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

// Forward declaration (platform-specific)
class FrameDisplayImpl;

/**
 * @class FrameDisplay
 * @brief Decodes and displays video frames
 * 
 * Platform Support:
 * - Linux: X11 + libjpeg
 * - Windows: GDI+ + libjpeg
 * - (Optional) OpenCV for cross-platform
 */
class FrameDisplay
{
private:
    std::unique_ptr<FrameDisplayImpl> impl_; ///< Platform-specific implementation

    int width_;
    int height_;
    std::string windowTitle_;

    std::thread displayThread_;
    std::atomic<bool> running_;

    mutable std::atomic<double> fps_;
    mutable std::atomic<int> frameCount_;
    mutable std::chrono::steady_clock::time_point fpsStartTime_;
    mutable std::chrono::steady_clock::time_point lastFrameTime_;

    /**
     * @brief Display loop (runs in thread)
     * 
     * Algorithm:
     * while (running_) {
     *     1. Pop frame from FrameBuffer (timeout 40ms)
     *     2. Decode JPEG to RGB
     *     3. Render to window
     *     4. Handle window events
     * }
     */
    void displayLoop();

public:
    /**
     * @brief Constructor
     * @param width Window width
     * @param height Window height
     * @param title Window title
     */
    FrameDisplay(int width = 640, int height = 480,
                const std::string& title = "RTSP Client");

    ~FrameDisplay();

    /**
    * @brief Create window and start display thread
    * @return true if successful
    */
    bool initialize();

    /**
    * @brief Stop display thread and close window
    */
    void shutdown();

    /**
     * @brief Display single frame (blocking)
     * @param jpegData JPEG image data
     * @return true if displayed, false if window closed
     * 
     * @details
     * 1. Decode JPEG to RGB bitmap
     * 2. Render to window
     * 3. Handle window events (close, resize)
     */
    bool displayFrame(const std::vector<uint8_t>& jpegData);

    /**
    * @brief Check if window is open
    */
    bool isOpen() const;

    /**
    * @brief Get current FPS
    */
    double getFPS() const;
};

#endif