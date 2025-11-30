#ifndef CLIENT_UI_CLIENTUI_HPP
#define CLIENT_UI_CLIENTUI_HPP

#include <opencv4/opencv2/opencv.hpp>

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "network/RTSPClient.hpp"
#include "rtp/RTPReceiver.hpp"
#include "rtp/FrameReassembler.hpp"
#include "buffer/FrameBuffer.hpp"
#include "ui/FrameDisplay.hpp"

#ifdef USE_OPENCV
    #include <opencv4/opencv2/opencv.hpp>   
    
    /**
     * @brief Button structure for GUI
     */
    struct Button
    {
        cv::Rect rect;
        std::string label;
        cv::Scalar color;
        cv::Scalar hoverColor;
        bool enabled;
        bool hovered; 
    };

    void mouseCallback(int event, int x, int y, int flags, void* userdata);
    cv::Mat createControlPanel(int width, int height, const std::string& status);
#endif

/**
 * @class ClientUI
 * @brief Main UI controller for RTSP client application
 * 
 * Manages:
 * - UI window with control buttons
 * - Video display area
 * - RTSP client connection
 * - RTP receiver and frame buffer
 * - Display thread
 */
class ClientUI
{
private:
    std::unique_ptr<RTSPClient> rtspClient_;
    std::unique_ptr<RTPReceiver> rtpReceiver_;
    std::unique_ptr<FrameReassembler> frameReassembler_;
    std::unique_ptr<FrameBuffer> frameBuffer_;
    std::unique_ptr<FrameDisplay> frameDisplay_;

    std::string serverIP_;
    int serverPort_;
    std::string videoFile_;
    int clientRTPPort_;

    bool initialized_;

public:
    /**
     * @brief Constructor
     * @param serverIP Server IP address
     * @param serverPort Server RTSP port
     * @param videoFile Video filename on server
     * @param clientRTPPort Local RTP port
     */
    ClientUI(const std::string& serverIP,
            int serverPort,
            const std::string& videoFile,
            int clientRTPPort = 25000);

    ~ClientUI();

    /**
    * @brief Initialize UI and components
    * @return true if successful
    */
    bool initialize();

    /**
    * @brief Run main UI loop (blocking)
    *
    * @details
    * - Creates window with buttons
    * - Handles button clicks
    * - Updates display
    * - Returns when window closed
    */
    void run();

    /**
    * @brief Handle SETUP button click
    */
    void onSetupButton();

    /**
    * @brief Handle PLAY button click
    */
    void onPlayButton();

    /**
    * @brief Handle PAUSE button click
    */
    void onPauseButton();

    /**
    * @brief Handle TEARDOWN button click
    */
    void onTeardownButton();

    /**
    * @brief Get current status string
    * @return Status text (e.g., "PLAYING - FPS: 25 - LOSS: 0.5%")
    */
    std::string getStatusString() const;
};
#endif