#include "ui/ClientUI.hpp"
#include "utils/Logger.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdint>

#ifdef USE_OPENCV
    #include <opencv4/opencv2/opencv.hpp>

    static std::vector<Button> g_buttons;
    static std::function<void(int)> g_buttonCallback;

    void mouseCallback(int event,
                        int x, int y,
                        int flags,
                        void* userdata){
        (void)flags;
        (void)userdata;
        if(event == cv::EVENT_MOUSEMOVE){
            for(size_t i = 0; i < g_buttons.size(); i++){
                g_buttons[i].hovered = g_buttons[i].rect.contains(cv::Point(x, y));
            }
        }
        else if(event == cv::EVENT_LBUTTONDOWN){
            for(size_t i = 0; i < g_buttons.size(); i++){
                if(g_buttonCallback){
                    g_buttonCallback(i);
                }
                break;
            }
        }
    }

    cv::Mat createControlPanel(int width, int height, const std::string& status)
    {
        cv::Mat panel(height, width, CV_8UC3, cv::Scalar(220, 220, 240));

        for(const auto& btn: g_buttons){
            cv::Scalar btnColor = btn.enabled ? (btn.hovered ? btn.hoverColor : btn.color) : cv::Scalar(180, 180, 180);

            cv::rectangle(panel, btn.rect, btnColor, -1);
            cv::rectangle(panel, btn.rect, cv::Scalar(80, 80, 80), 2, cv::LINE_AA);

            int baseline = 0;
            cv::Size textSize = cv::getTextSize(btn.label, cv::FONT_HERSHEY_DUPLEX, 0.4, 2, &baseline);
            cv::Point textOrg(
                btn.rect.x + (btn.rect.width - textSize.width) / 2,
                btn.rect.y + (btn.rect.height - textSize.height) / 2 - 3
            );

            cv::putText(panel, btn.label, textOrg, cv::FONT_HERSHEY_DUPLEX,
                        0.4, cv::Scalar(40, 40, 40), 2, cv::LINE_AA);
            
            cv::rectangle(panel, cv::Rect(10, height - 40, width - 20, 30),
                        cv::Scalar(50, 50, 50), -1);

            cv::putText(panel, status, cv::Point(15, height - 15),
                        cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
        }
        return panel;
    }
#endif

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

        if(!frameDisplay_->initialize()){
            Logger::getInstance().log(LogLevel::ERROR, "Failed to initialize frame display");
            return false;
        }

        initialized_ = true;
        Logger::getInstance().log(LogLevel::INFO, "Client UI initialized successfully");

        return true;
    }catch(const std::exception& e){
        Logger::getInstance().log(LogLevel::ERROR,
            "Failed to initialize Client UI: " + std::string(e.what()));
        return false;
    }
}

void ClientUI::run()
{
    if(!initialized_){
        std::cerr << "Error: ClientUI not initialized. Call initialize() first.\n";
        return;
    }

    std::cout << "\n========================================\n";
    std::cout << "   RTSP Video Streaming Client\n";
    std::cout << "========================================\n";
    std::cout << "Server: " << serverIP_ << ":" << serverPort_ << "\n";
    std::cout << "Video:  " << videoFile_ << "\n";
    std::cout << "RTP Port: " << clientRTPPort_ << "\n";
    std::cout << "========================================\n\n";

    #ifdef USE_OPENCV
        const std::string controlWindowName = "RTSP Client Controls";
        const int controlWidth = 640;
        const int controlHeight = 150;

        cv::namedWindow(controlWindowName, cv::WINDOW_NORMAL);
        cv::resizeWindow(controlWindowName, controlWidth, controlHeight);

        g_buttons.clear();
        int btnWidth = 140;
        int btnHeight = 50;
        int spacing = 20;
        int startX = 20;
        int startY = 20;

        g_buttons.push_back({
            cv::Rect(startX, startY, btnWidth, btnHeight),
            "SETUP",
            cv::Scalar(67, 86, 99), 
            cv::Scalar(49, 54, 71), 
            true, false
        });

        g_buttons.push_back({
            cv::Rect(startX + btnWidth + spacing, startY, btnWidth, btnHeight),
            "PLAY",
            cv::Scalar(67, 86, 99), 
            cv::Scalar(49, 54, 71), 
            true, false
        });

        g_buttons.push_back({
            cv::Rect(startX + 2*(btnWidth + spacing), startY, btnWidth, btnHeight),
            "PAUSE",
            cv::Scalar(67, 86, 99), 
            cv::Scalar(49, 54, 71), 
            true, false
        });

        g_buttons.push_back({
            cv::Rect(startX + 3*(btnWidth + spacing), startY, btnWidth, btnHeight),
            "TEARDOWN",
            cv::Scalar(67, 86, 99), 
            cv::Scalar(49, 54, 71), 
            true, false
        });

        g_buttonCallback = [this](int buttonIndex){
            switch(buttonIndex){
                case 0: onSetupButton(); break;
                case 1: onPlayButton(); break;
                case 2: onPauseButton(); break;
                case 3: onTeardownButton(); break;
            }
        };
        
        cv::setMouseCallback(controlWindowName, mouseCallback, nullptr);

        std::cout << "GUI Mode: Click buttons to control playback\n";
        std::cout << "Press 'Q' in any window to quit\n";
        std::cout << "========================================\n\n";

        bool running = true;

        std::thread displayThread([this, &running](){
            while(running && frameDisplay_->isOpen()){
                std::vector<uint8_t> frame;
                bool succes = frameBuffer_->pop(frame, 100);

                if(succes && !frame.empty()){
                    if(!frameDisplay_->displayFrame(frame)){
                        running = false;
                        break;
                    }
                }
            }
        });

        auto lastStatusUpdate = std::chrono::steady_clock::now();

        while(running){
            std::string status = getStatusString();

            cv::Mat controlPanel = createControlPanel(controlWidth, controlHeight, status);
            cv::imshow(controlWindowName, controlPanel);

            int key = cv::waitKey(30) & 0xFF;

            if(key == 'q' || key == 'Q' || key == 27){
                running = false;
                break;
            }

            // Check if window is still open
            if(cv::getWindowProperty(controlWindowName, cv::WND_PROP_VISIBLE) < 1){
                running = false;
                break;
            }

            if(!frameDisplay_->isOpen()){
                running = false;
                break;
            }

            auto now = std::chrono::steady_clock::now();
            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastStatusUpdate).count() >= 1){
                std::cout << "\r" << status << std::flush;
                lastStatusUpdate = now;
            }
        }
        running = false;

        onTeardownButton();
        frameDisplay_->shutdown();
        cv::destroyWindow(controlWindowName);

        if(displayThread.joinable()){
            displayThread.join();
        }

        std::cout << "\n[ClientUI] Exisiting...\n";
    #else
        std::cout << "Terminal Mode (OpenCV not available)\n";
        std::cout << "Commands:\n";
        std::cout << "  s - SETUP | p - PLAY | a - PAUSE | t - TEARDOWN | q - QUIT\n";
        std::cout << "========================================\n\n";

        bool running = true;
        
        // Terminal input loop
        std::cout << "Enter command: ";
        std::string cmd;
        while (running && std::cin >> cmd) {
            if (cmd == "s" || cmd == "S") {
                onSetupButton();
            } else if (cmd == "p" || cmd == "P") {
                onPlayButton();
            } else if (cmd == "a" || cmd == "A") {
                onPauseButton();
            } else if (cmd == "t" || cmd == "T") {
                onTeardownButton();
            } else if (cmd == "q" || cmd == "Q") {
                running = false;
                break;
            } else {
                std::cout << "Unknown command: " << cmd << "\n";
            }
            
            std::cout << getStatusString() << "\n";
            std::cout << "Enter command: ";
        }
        
        onTeardownButton();
        std::cout << "\n[ClientUI] Exiting...\n";
    #endif
}

void ClientUI::onSetupButton()
{
    try{
        Logger::getInstance().log(LogLevel::INFO, "SETUP button clicked");
        std::cout << "\n[SETUP] Connecting to server ...\n";

        bool succes = rtspClient_->sendSetup(videoFile_, clientRTPPort_);

        if(succes){
            std::string sessionId = rtspClient_->getSessionId();
            std::cout << "[SETUP] Success! Session ID: " << sessionId << "\n";
            Logger::getInstance().log(LogLevel::INFO, "SETUP successful, Session ID: " + sessionId);
        } else{
            std::cout << "[SETUP] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "SETUP Failed");
        }
    }catch(const std::exception& e){
        Logger::getInstance().log(LogLevel::ERROR, std::string("SETUP failed: ") + e.what());
        std::cerr << "[SETUP] Error: " << e.what() << '\n';
    }
}

void ClientUI::onPlayButton()
{
    try{
        Logger::getInstance().log(LogLevel::INFO, "PLAY button clicked");
        std::cout << "\n[PLAY] Starting playback ...\n";

        bool succes = rtspClient_->sendPlay();

        if(succes){
            rtpReceiver_->start();
            std::cout << "[PLAY] Streaming started!" << "\n";
            Logger::getInstance().log(LogLevel::INFO, "PLAY successful");
        } else{
            std::cout << "[PLAY] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "PLAY Failed");
        }
    }catch(const std::exception& e){
        Logger::getInstance().log(LogLevel::ERROR, std::string("PLAY failed: ") + e.what());
        std::cerr << "[PLAY] Error: " << e.what() << '\n';
    }
}

void ClientUI::onPauseButton()
{
    try{
        Logger::getInstance().log(LogLevel::INFO, "PAUSE button clicked");
        std::cout << "\n[Pause] Pausing playback ...\n";

        bool succes = rtspClient_->sendPause();

        if(succes){
            rtpReceiver_->stop();
            std::cout << "[PAUSE] Streaming paused!" << "\n";
            Logger::getInstance().log(LogLevel::INFO, "PAUSE successful");
        } else{
            std::cout << "[PAUSE] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "PAUSE Failed");
        }
    }catch(const std::exception& e){
        Logger::getInstance().log(LogLevel::ERROR, std::string("PAUSE failed: ") + e.what());
        std::cerr << "[PAUSE] Error: " << e.what() << '\n';
    }
}

void ClientUI::onTeardownButton()
{
    try{
        Logger::getInstance().log(LogLevel::INFO, "TEARDOWN button clicked");
        std::cout << "\n[TEARDOWN] Ending session ...\n";

        rtpReceiver_->stop();

        bool succes = rtspClient_->sendTeardown();

        if(succes){
            std::cout << "[TEARDOWN] Session ended!" << "\n";
            Logger::getInstance().log(LogLevel::INFO, "TEARDOWN successful");
        } else{
            std::cout << "[TEARDOWN] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "TEARDOWN Failed");
        }
    }catch(const std::exception& e){
        Logger::getInstance().log(LogLevel::ERROR, std::string("TEARDOWN failed: ") + e.what());
        std::cerr << "[TEARDOWN] Error: " << e.what() << '\n';
    }
}

std::string ClientUI::getStatusString() const
{
    std::ostringstream oss;
    
    std::string state = "INIT";
    if(rtspClient_){
        if(rtpReceiver_ && rtpReceiver_->isRunning()) state = "PLAYING";
        else state = "READY";
    }

    double fps = frameDisplay_ ? frameDisplay_->getFPS() : 0.0;

    double packetLoss = 0.0;
    uint64_t packetsReceived = 0;

    if(rtpReceiver_){
        packetLoss = rtpReceiver_->getPacketLossPercentage();
        packetsReceived = rtpReceiver_->getPacketReceived();
    }

    oss << "[" << state << "] "
        << "FPS: " << std::fixed << std::setprecision(1) << fps << " | "
        << "Packets: " << packetsReceived << " | "
        << "Loss: " << std::fixed << std::setprecision(2) << packetLoss << "%";
    
    return oss.str();
}