#ifndef SERVER_NETWORK_SERVERWORKER_HPP
#define SERVER_NETWORK_SERVERWORKER_HPP

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include<random>
#include "utils/Logger.hpp"
#include "network/RTSPMessage.hpp"
#include "network/Socket.hpp"
#include "rtp/EncodingStrategy.hpp"
#include "utils/Timer.hpp"
#include "video/VideoStream.hpp"

/**
 * @class ServerWorker
 * @brief Worker thread handler for processing individual client RTSP requests
 *
 * @details
 * ServerWorker handles:
 * - Processing RTSP commands (SETUP, PLAY, PAUSE, TEARDOWN) from one client
 * - Sending RTP video packets to client
 * - Managing client session state
 * - Cleanup on disconnect
 */
class ServerWorker {
  public:
    /**
     * @enum State
     * @brief RTSP session state
     */
    enum class State { INIT, READY, PLAYING, PAUSED };

  private:
    int clientId_;
    std::unique_ptr<Socket> socket_;

    std::string clientIP_;
    int clientRTPPort_;

    State state_;
    std::string sessionId_;
    std::atomic<bool> running_;

    std::unique_ptr<VideoStream> videoStream_;            ///< Video file render
    std::unique_ptr<EncodingStrategy> encodingStrategy_;  ///< RTP packet encoder
    std::unique_ptr<Timer> frameTimer_;  ///< Frame rate controller (25 fps = 40ms interval)

    uint16_t sequenceNumber_;
    uint32_t timestamp_;
    uint32_t ssrc_;  ///< Synchronization source identifier

    std::thread streamingThread_;  ///< Background thread for RTP streaming
    std::atomic<bool> streaming_;  ///< Streaming active flag

    /**
     * @brief RTSP request handler (main loop)
     *
     * Algorithm:
     * 1. Receive RTSP request from client (TCP)
     * 2. Parse request using RTSPMessage
     * 3. Dispatch to appropriate handler
     * 4. Send RTSP response
     * 5. Repeat until TEARDOWN or error
     */
    void handleRtspRequests();

    /**
     * @brief Handle SETUP command
     * @param request Parsed RTSP request
     * @return RTSP response string
     *
     * Actions:
     * - Generate session ID
     * - Open video file
     * - Create RTP socket
     * - Extract client RTP port from Transport header
     * - Transition: INIT → READY
     */
    std::string handleSetup(const RTSPMessage::Request& request);

    /**
     * @brief Handle PLAY command
     * @param request Parsed RTSP request
     * @return RTSP response string
     *
     * Actions:
     * - Validate session ID
     * - Start streaming thread
     * - Transition: READY → PLAYING
     */
    std::string handlePlay(const RTSPMessage::Request& request);

    /**
     * @brief Handle PAUSE command
     * @param request Parsed RTSP request
     * @return RTSP response string
     *
     * Actions:
     * - Stop streaming thread
     * - Transition: PLAYING → READY
     */
    std::string handlePause(const RTSPMessage::Request& request);

    /**
     * @brief Handle TEARDOWN command
     * @param request Parsed RTSP request
     * @return RTSP response string
     *
     * Actions:
     * - Stop streaming
     * - Close video file
     * - Cleanup resources
     * - Transition: Any → INIT
     */
    std::string handleTeardown(const RTSPMessage::Request& request);

    /**
     * @brief RTP streaming loop (runs in separate thread)
     *
     * Algorithm:
     * while (streaming_) {
     *     1. timer.start()
     *     2. Read frame from VideoStream
     *     3. Encode frame to RTP packets (EncodingStrategy)
     *     4. Send each packet via UDP to client
     *     5. Increment sequence number
     *     6. Update timestamp
     *     7. timer.wait()  // Maintain 25 fps (40ms)
     * }
     *
     * Handles:
     * - Frame rate control (25 fps)
     * - Sequence number wrap-around
     * - EOF (loop or stop)
     */
    void streamingLoop();

    /**
     * @brief Generate unique session ID
     * @return Session ID string (e.g., "123456789")
     */
    std::string generateSessionId();

    /**
     * @brief Validate session ID from request
     * @param request RTSP request
     * @return true if session ID matches, false otherwise
     */
    bool validateSessionId(const RTSPMessage::Request& request) const;

public:
    /**
    * @brief Constructor
    * @param clientId Client ID
    * @param rtspSocket TCP socket for RTSP communication
    */
    ServerWorker(int clientId, std::unique_ptr<Socket> rtspSocket);

    /**
    * @brief Destructor - cleanup resources
    */
    ~ServerWorker();

    /**
    * @brief Main worker entry point
    *
    * Called by RTSPServer in separate thread
    * Runs handleRtspRequests() until client disconnects or error
    */
    void run();

    /**
    * @brief Stop worker gracefully
    */
    void stop();

    /**
    * @brief Get client ID
    */
    int getClientId() const { return clientId_; }

    /**
    * @brief Get current state
    */
    State getState() const { return state_; }
};
#endif