#include "ui/ClientUI.hpp"
#include "utils/Logger.hpp"

#ifdef USE_OPENCV
    #include <opencv2/opencv.hpp>
#endif

#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

ClientUI::ClientUI(const std::string& serverIP,
                    int serverPort,
                    const std::string& videoFile,
                    int clientRTPPort) 
                    : 
                    serverIP_(serverIP),
                    serverPort_(serverPort),
                    videoFile_(videoFile),
                    clientRTPPort_(clientRTPPort),
                    initialized_(false){}

ClientUI::~ClientUI(){
    if (rtpReceiver_) {
        rtpReceiver_->stop();
    }
    
    if (frameDisplay_) {
        frameDisplay_->shutdown();
    }
    
    frameDisplay_.reset();
    frameBuffer_.reset();
    frameReassembler_.reset();
    rtpReceiver_.reset();
    rtspClient_.reset();
}

bool ClientUI::initialize(){
    try{
        Logger::getInstance().log(LogLevel::INFO,
            "Initializing Client UI ...");

        rtspClient_ = std::make_unique<RTSPClient>(serverIP_, serverPort_);

        ///> Create Buffer (max 30 frames)
        frameBuffer_ = std::make_unique<FrameBuffer>(30);

        frameReassembler_ = std::make_unique<FrameReassembler>(frameBuffer_.get());
        
        rtpReceiver_ = std::make_unique<RTPReceiver>(
            clientRTPPort_,
            frameReassembler_.get()
        );

        frameDisplay_ = std::make_unique<FrameDisplay>(640, 480, "RTSP Video Player");

        
    }catch(const std::exception& e){
        Logger::getInstance().log(LogLevel::ERROR,
            "Failed to initialize Client UI: " + std::string(e.what()));
        return false;
    }

    return true;
}