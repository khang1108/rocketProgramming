#ifndef FRAME_DISPLAY_HPP
#define FRAME_DISPLAY_HPP

#include <QLabel>
#include <vector>
#include <atomic>
#include <chrono>

class FrameDisplay : public QLabel
{
    Q_OBJECT

public:
    FrameDisplay(int width, int height, const char* title, QWidget* parent = nullptr);
    ~FrameDisplay();
    bool initialize();
    void shutdown();
    bool displayFrame(const std::vector<uint8_t>& jpegData);
    bool isOpen() const;
    double getFPS() const;

private:
    int width_;
    int height_;
    bool running_;

    std::atomic<double> fps_;
    std::atomic<int> frameCount_;
    std::chrono::steady_clock::time_point fpsStartTime_;
};

#endif