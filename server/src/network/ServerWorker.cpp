#include "network/ServerWorker.hpp"
#include <random>
#include "rtp/EncodingStrategy.hpp"
#include "utils/Logger.hpp"

#ifdef ERROR
#undef ERROR
#endif

void ServerWorker::handleRtspRequests() {
    char buffer[4096];

    while (running_) {
        try {
            if (!socket_) {
                LOG_INFO("Worker " + std::to_string(clientId_) + ": socket closed");
                break;
            }
            // Ép kiểu 'char*' sang 'uint8_t*'
            int bytesRead = socket_->receive(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));

            if (bytesRead <= 0) {
                LOG_INFO("Client " + std::to_string(clientId_) + " disconnected.");
                break;
            }

            std::string rawRequest(buffer, bytesRead);

            try {
                LOG_INFO("Received RTSP request (" + std::to_string(bytesRead) + " bytes)");

                auto request = RTSPMessage::parseRequest(rawRequest);
                LOG_INFO("Parsed request: " + request.method + " " + request.url);

                std::string response;

                if (request.method == "SETUP")
                    response = handleSetup(request);
                else if (request.method == "PLAY")
                    response = handlePlay(request);
                else if (request.method == "PAUSE")
                    response = handlePause(request);
                else if (request.method == "TEARDOWN") {
                    response = handleTeardown(request);
                    running_ = false;
                } else {
                    response = RTSPMessage::buildResponse(400, "Bad Request", request.cseq);
                }

                // Ép kiểu chuỗi (char*) sang byte (uint8_t*) để gửi đi
                socket_->send(reinterpret_cast<const uint8_t*>(response.c_str()),
                              response.length());
                LOG_INFO("Sent response: " + std::to_string(response.length()) + " bytes");

            } catch (const std::exception& e) {
                LOG_ERROR("RTSP Handling Error: " + std::string(e.what()));
                std::string errorResponse =
                    RTSPMessage::buildResponse(500, "Internal Server Error", 0);
                try {
                    socket_->send(reinterpret_cast<const uint8_t*>(errorResponse.c_str()),
                                  errorResponse.length());
                } catch (...) {
                }
            }
        } catch (const SocketException& e) {
            LOG_INFO("Worker " + std::to_string(clientId_) + ": socket closed");
            break;
        } catch (const std::exception& e) {
            LOG_ERROR("Worker " + std::to_string(clientId_) + " error " + e.what());
            break;
        }
    }
}

std::string ServerWorker::handleSetup(const RTSPMessage::Request& request) {
    ///< Ensure that the state == State::INIT -> State::READY
    if (state_ != State::INIT)
        return RTSPMessage::buildResponse(400, "Invalid State", request.cseq);

    try {
        std::string transport = request.headers.at("Transport");
        size_t pos = transport.find("client_port=");
        if (pos != std::string::npos) {
            std::string portStr = transport.substr(pos + 12);
            size_t dash = portStr.find('-');
            if (dash != std::string::npos)
                portStr = portStr.substr(0, dash);
            clientRTPPort_ = std::stoi(portStr);
        }
    } catch (...) {
        return RTSPMessage::buildResponse(400, "Bad Transport Header", request.cseq);
    }

    // Parse RTSP URI to extract file path
    // URL can be: "rtsp://127.0.0.1:8554/videos/movie.Mjpeg" or just "videos/movie.Mjpeg"
    std::string filePath = request.url;

    // If URL starts with rtsp://, extract the path after domain:port
    if (filePath.find("rtsp://") == 0) {
        size_t slashPos = filePath.find('/', 7);  // Skip "rtsp://"
        if (slashPos != std::string::npos) {
            filePath = filePath.substr(slashPos + 1);  // Get path after first /
        }
    }

    LOG_INFO("SETUP request for file: " + filePath);

    try {
        videoStream_ = std::make_unique<VideoStream>(filePath);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to open video file '" + filePath + "': " + std::string(e.what()));
        return RTSPMessage::buildResponse(404, "File Not Found", request.cseq);
    }

    sessionId_ = generateSessionId();
    state_ = State::READY;

    LOG_INFO("SETUP successful, Session ID: " + sessionId_);

    return RTSPMessage::buildResponse(
        200, "OK", request.cseq,
        {{"Session", sessionId_},
         {"Transport", "RTP/AVP/UDP;unicast;client_port=" + std::to_string(clientRTPPort_) + "-" +
                           std::to_string(clientRTPPort_ + 1) + ";server_port=0-0"}});
}

std::string ServerWorker::handlePlay(const RTSPMessage::Request& request) {
    if (state_ != State::READY && state_ != State::PAUSED)
        return RTSPMessage::buildResponse(400, "Invalid State", request.cseq);

    if (!validateSessionId(request))
        return RTSPMessage::buildResponse(454, "Session Not Found", request.cseq);

    if (!streaming_) {
        streaming_ = true;
        streamingThread_ = std::thread(&ServerWorker::streamingLoop, this);
    }

    state_ = State::PLAYING;
    return RTSPMessage::buildResponse(200, "OK", request.cseq, {{"Session", sessionId_}});
}

std::string ServerWorker::handlePause(const RTSPMessage::Request& request) {
    if (state_ != State::PLAYING)
        return RTSPMessage::buildResponse(400, "Invalid State", request.cseq);
    if (!validateSessionId(request))
        return RTSPMessage::buildResponse(454, "Session Not Found", request.cseq);

    state_ = State::READY;
    return RTSPMessage::buildResponse(200, "OK", request.cseq, {{"Session", sessionId_}});
}

std::string ServerWorker::handleTeardown(const RTSPMessage::Request& request) {
    streaming_ = false;
    if (streamingThread_.joinable())
        streamingThread_.join();

    videoStream_.reset();
    state_ = State::INIT;
    return RTSPMessage::buildResponse(200, "OK", request.cseq, {{"Session", sessionId_}});
}

void ServerWorker::streamingLoop() {
    // Tạo socket UDP mới để gửi RTP
    Socket rtpSocket(SocketType::UDP);  // Sử dụng Enum SocketType::UDP

    // IP Client (Lấy từ socket TCP hoặc hardcode localhost để test)
    // clientIP_ = socket_->getPeerAddress();
    clientIP_ = socket_->getPeerAddress();

    LOG_INFO("Streaming to " + clientIP_ + ":" + std::to_string(clientRTPPort_));

    // ===== HD ENCODING: Fragmentation Strategy =====
    // Fragment frames thành nhiều RTP packets (mỗi packet ≤1400 bytes)
    // → Tránh MTU limit, đảm bảo frame hoàn chỉnh
    auto encodingStrategy = std::make_unique<HDEncodingStrategy>();
    LOG_INFO("Using HDEncodingStrategy (fragmentation enabled, max payload: 1400 bytes)");

    while (streaming_) {
        if (state_ != State::PLAYING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        frameTimer_->start();

        if (videoStream_->hasMoreFrames()) {
            try {
                auto frameData = videoStream_->nextFrame();

                // Tạo Frame object và encode với HDEncodingStrategy
                Frame frame(frameData, sequenceNumber_, timestamp_, ssrc_);
                auto packets = encodingStrategy->execute(frame);

                // ===== HD STREAMING METRICS =====
                static int frameCount = 0;
                static size_t totalBytes = 0;
                static int totalPackets = 0;

                frameCount++;
                totalBytes += frameData.size();
                totalPackets += packets.size();

                // Log mỗi 25 frames (1 giây với 25 FPS)
                if (frameCount % 25 == 0) {
                    double avgFrameSize = (double)totalBytes / frameCount;
                    double avgPacketsPerFrame = (double)totalPackets / frameCount;

                    LOG_INFO("HD Streaming Stats [" + std::to_string(frameCount) + " frames]:");
                    LOG_INFO("  ├─ Current frame: " + std::to_string(frameData.size()) +
                             " bytes → " + std::to_string(packets.size()) + " packets");
                    LOG_INFO("  ├─ Average frame size: " + std::to_string((int)avgFrameSize) +
                             " bytes");
                    LOG_INFO("  └─ Average packets/frame: " + std::to_string(avgPacketsPerFrame));
                }

                // Gửi tất cả packets qua UDP
                // HD: Mỗi packet ≤1400 bytes, nhiều packets = 1 frame hoàn chỉnh
                for (const auto& packet : packets) {
                    auto data = packet.getPacketVector();
                    rtpSocket.sendTo(data.data(), data.size(), clientIP_, clientRTPPort_);
                }

                sequenceNumber_ += packets.size();
                timestamp_ += 3600;

            } catch (const std::exception& e) {
                LOG_ERROR("Streaming error: " + std::string(e.what()));
                break;
            }
        } else {
            LOG_INFO("Video ended, rewinding for loop playback...");
            videoStream_->rewind();
            // Reset timestamp cho loop mới (optional - giữ sequence number liên tục)
            // timestamp_ = 0;  // Uncomment nếu muốn reset timestamp
        }

        frameTimer_->wait();
    }
}

std::string ServerWorker::generateSessionId() {
    static int idCounter = 100000;
    return std::to_string(idCounter++);
}

bool ServerWorker::validateSessionId(const RTSPMessage::Request& request) const {
    auto it = request.headers.find("Session");
    if (it != request.headers.end())
        return it->second == sessionId_;
    return false;
}

ServerWorker::ServerWorker(int clientId, std::unique_ptr<Socket> rtspSocket)
    : clientId_(clientId),
      socket_(std::move(rtspSocket)),
      state_(State::INIT),
      running_(true),
      sequenceNumber_(0),  // ← Đưa lên trước
      timestamp_(0),       // ← Đúng thứ tự
      ssrc_(0),
      streaming_(false) {
    // Set timeout for client socket to keep connection alive between requests
    if (socket_) {
        socket_->setTimeout(30000);  // 30 seconds timeout for RTSP requests
        LOG_INFO("Worker " + std::to_string(clientId_) + " socket timeout set to 30s");
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(100000, 999999);
    ssrc_ = dis(gen);

    // Khởi tạo timer cho 25 FPS (40ms)
    frameTimer_ = std::make_unique<Timer>(40);
}

ServerWorker::~ServerWorker() {
    stop();
}

void ServerWorker::run() {
    running_ = true;
    LOG_INFO("Worker " + std::to_string(clientId_) + " started.");

    try {
        handleRtspRequests();
    } catch (const std::exception& e) {
        LOG_ERROR("Worker " + std::to_string(clientId_) + " error: " + e.what());
    }

    stop();
    LOG_INFO("Worker " + std::to_string(clientId_) + " stopped.");
}

void ServerWorker::stop() {
    static std::atomic<bool> stopCalled{false};
    if (stopCalled.exchange(true)) {
        return;
    }

    running_ = false;
    streaming_ = false;

    if (streamingThread_.joinable()) {
        streamingThread_.join();
    }

    if (socket_) {
        try {
            socket_->close();
        } catch (...) {
        }
        socket_.reset();
    }
}