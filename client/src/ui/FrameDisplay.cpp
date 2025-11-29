#include "ui/FrameDisplay.hpp"

#ifdef USE_OPENCV
    #include <opencv2/opencv.hpp>
#endif

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstring>

FrameDisplay::FrameDisplay(int width, int height, 
                            const std::string& title) : 
                            width_(width),
                            height_(height),
                            windowTitle_(title),
                            running_(false){}

FrameDisplay::~FrameDisplay(){
    shutdown();
}

bool FrameDisplay::initialize() {
    #ifdef USE_OPENCV
        try{
            cv::namedWindow(windowTitle_, cv::WINDOW_NORMAL);
            cv::resizeWindow(windowTitle_, width_, height_);

            running_ = true;
            std::cout << "[FrameDisplay] OpenCV window created: " 
                    << windowTitle_ << std::endl;

            return true;
        } catch(const cv::Exception& e){
            std::cerr << "[FrameDisplay] OpenCV error: " 
                << e.what() << std::endl;
            return false;
        }
    #else
        std::cout << "[FrameDisplay] OpenCV not available - please read README.md for installation instructions." << std::endl;
        return false;
    #endif
}

void FrameDisplay::shutdown() {
    running_ = false;
    if(displayThread_.joinable()){
        displayThread_.join();
    }

    #ifdef USE_OPENCV
        try{
            cv::destroyWindow(windowTitle_);
        } catch(const cv::Exception& e){
            std::cerr << "[FrameDisplay] OpenCV error: " 
                << e.what() << std::endl;
        }
    #endif
}

bool FrameDisplay::displayFrame(const std::vector<uint8_t>& jpegData){
    if(!running_){
        return false;
    }

    if(jpegData.empty()){
        std::cerr << "[FrameDisplay] Empty frame data" << std::endl;
        return false;
    }

    #ifdef USE_OPENCV
        try{
            cv::Mat frame = cv::imdecode(jpegData, cv::IMREAD_COLOR);

            if(frame.empty()){
                std::cerr << "[FrameDisplay] Failed to decode JEPG ("
                    << jpegData.size() << " bytes)" << std::endl;
                return false;
            }

            cv::imshow(windowTitle_, frame);

            rameCount_++;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - fpsStartTime_
            ).count();

            // Update FPS every 1 second (1000ms)
            if (elapsed >= 1000) {
                fps_ = (frameCount_.load() * 1000.0) / elapsed;
                frameCount_ = 0;
                fpsStartTime_ = now;
            }

            lastFrameTime_ = now;

            int key = cv::waitKey(1);

            if(key == 27 || cv::getWindowProperty(windowTitle_, cv::WND_PROP_VISIBLE) < 1){
                running_ = false;
                return false;
            }

            return true;
        }
        catch(const cv::Exception& e){
            std::cerr << "[FrameDisplay] OpenCV exception: " 
                    << e.what() << std::endl;
            return false;
        }
    #endif
}

bool FrameDisplay::isOpen() const{
    #ifdef USE_OPENCV
        try{
            return running_ &&
                cv::getWindowProperty(windowTitle_, cv::WND_PROP_VISIBLE) >= 1;
        }
        catch(const cv::Exception& e){
            std::cerr << "[FrameDisplay] OpenCV exception: " 
                    << e.what() << std::endl;
            return false;
        }
    #else
        return running_;
    #endif
}

double FrameDisplay::getFPS() const{
    return fps_.load();
}