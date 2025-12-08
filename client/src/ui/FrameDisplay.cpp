#include "ui/FrameDisplay.hpp"
#include "utils/Logger.hpp"

#include <QImage>
#include <QPixmap>
#include <QSizePolicy>
#include <iostream>

FrameDisplay::FrameDisplay(int width,
                            int height,
                            const char* title,
                            QWidget* parent) 
                        :
                        QLabel(parent),
                        width_(width),
                        height_(height),
                        running_(false),
                        fps_(0.0),
                        frameCount_(0),
                        fpsStartTime_(std::chrono::steady_clock::now())
{
    setText("No Video Found");
    setAlignment(Qt::AlignCenter);
    setMinimumSize(width_, height_);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background-color: black; color: white;");
    setWindowTitle(title);
}

FrameDisplay::~FrameDisplay()
{
    shutdown();
}

bool FrameDisplay::initialize()
{
    running_ = true;
    show();

    std::cout << "[FrameDisplay] Qt Label Create\n";
    Logger::getInstance().log(LogLevel::INFO,
                        "[FrameDisplay] Qt Label Created: " +
                        windowTitle().toStdString() + "\n");
    return true;
}

void FrameDisplay::shutdown()
{
    running_ = false;
    hide();
}

bool FrameDisplay::displayFrame(const std::vector<uint8_t>& jpegData)
{
    if(!running_) return false;

    if(jpegData.empty()){
        std::cerr << "[FrameDisplay] Empty frame data\n";
        Logger::getInstance().log(LogLevel::ERROR, 
                                "[FrameDisplay] Empty frame data\n");
        return false;
    }

    QImage image;
    if(!image.loadFromData(jpegData.data(),
                            static_cast<int>(jpegData.size()), "JPEG")){
        std::cerr << "[FrameDisplay] Failed to decode JPEG ("
                << jpegData.size() << " bytes)\n";
        Logger::getInstance().log(LogLevel::ERROR, 
            "[FrameDisplay] Failed to decode JPEG (" + 
            std::to_string(jpegData.size()) + 
            " bytes)\n"
        );
        return false;
    }

    frameCount_++;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - fpsStartTime_).count();
    if(elapsed >= 1000){
        fps_ = (frameCount_.load() * 1000.0) / elapsed;
        frameCount_ = 0;
        fpsStartTime_ = now;
    }

    QPixmap pixmap = QPixmap::fromImage(
        image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)
    );

    setPixmap(pixmap);
    return true;
}

bool FrameDisplay::isOpen() const
{
    return running_ && isVisible();
}
double FrameDisplay::getFPS() const
{
    return fps_.load();
}