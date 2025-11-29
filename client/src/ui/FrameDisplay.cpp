#include "ui/FrameDisplay.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

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

}

void FrameDisplay::shutdown() {
    running_ = false;
    if(displayThread_.joinable()){
        displayThread_.join();
    }
}

bool FrameDisplay::displayFrame(const std::vector<uint8_t>& jpegData){

}