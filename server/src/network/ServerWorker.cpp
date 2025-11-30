#include "ServerWorker.hpp"

#ifdef ERROR
#undef ERROR
#endif

void ServerWorker::handleRtspRequests() 
{
    char buffer[4096];

    while (running_) {
        //Ép kiểu 'char*' sang 'uint8_t*'
        int bytesRead = socket_->receive(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));

        if (bytesRead <= 0) {
            LOG_INFO("Client " + std::to_string(clientId_) + " disconnected.");
            break;
        }

        std::string rawRequest(buffer, bytesRead);
        
        try {
            auto request = RTSPMessage::parseRequest(rawRequest);
            std::string response;

            if (request.method == "SETUP") response = handleSetup(request);
            else if (request.method == "PLAY") response = handlePlay(request);
            else if (request.method == "PAUSE") response = handlePause(request);
            else if (request.method == "TEARDOWN") {
                response = handleTeardown(request);
                running_ = false; 
            } else {
                response = RTSPMessage::buildResponse(400, "Bad Request", request.cseq);
            }

            //Ép kiểu chuỗi (char*) sang byte (uint8_t*) để gửi đi
            socket_->send(reinterpret_cast<const uint8_t*>(response.c_str()), response.length());

        } catch (const std::exception& e) {
            LOG_ERROR("RTSP Handling Error: " + std::string(e.what()));
        }
    }
}

std::string ServerWorker::handleSetup(const RTSPMessage::Request& request) {
    if (state_ != State::INIT) return RTSPMessage::buildResponse(400, "Invalid State", request.cseq);

    try {
        std::string transport = request.headers.at("Transport");
        size_t pos = transport.find("client_port=");
        if (pos != std::string::npos) {
            std::string portStr = transport.substr(pos + 12);
            size_t dash = portStr.find('-');
            if (dash != std::string::npos) portStr = portStr.substr(0, dash);
            clientRTPPort_ = std::stoi(portStr);
        }
    } catch (...) {
        return RTSPMessage::buildResponse(400, "Bad Transport Header", request.cseq);
    }

    try {
        videoStream_ = std::make_unique<VideoStream>(request.url);
    } catch (...) {
        return RTSPMessage::buildResponse(404, "File Not Found", request.cseq);
    }

    sessionId_ = generateSessionId();
    state_ = State::READY;

    return RTSPMessage::buildResponse(200, "OK", request.cseq, {
        {"Session", sessionId_},
        {"Transport", "RTP/UDP; client_port=" + std::to_string(clientRTPPort_)}
    });
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
    if (state_ != State::PLAYING) return RTSPMessage::buildResponse(400, "Invalid State", request.cseq);
    if (!validateSessionId(request)) return RTSPMessage::buildResponse(454, "Session Not Found", request.cseq);

    state_ = State::READY; 
    return RTSPMessage::buildResponse(200, "OK", request.cseq, {{"Session", sessionId_}});
}

std::string ServerWorker::handleTeardown(const RTSPMessage::Request& request) {
    streaming_ = false;
    if (streamingThread_.joinable()) streamingThread_.join();
    
    videoStream_.reset();
    state_ = State::INIT;
    return RTSPMessage::buildResponse(200, "OK", request.cseq, {{"Session", sessionId_}});
}

void ServerWorker::streamingLoop() 
{
    //Tạo socket UDP mới để gửi RTP
    Socket rtpSocket(SocketType::UDP); //Sử dụng Enum SocketType::UDP
    
    //IP Client (Lấy từ socket TCP hoặc hardcode localhost để test)
    //clientIP_ = socket_->getPeerAddress(); 
    clientIP_ = "127.0.0.1"; 

    LOG_INFO("Streaming to " + clientIP_ + ":" + std::to_string(clientRTPPort_));

    while (streaming_) {
        if (state_ != State::PLAYING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        frameTimer_->start();

        if (videoStream_->hasMoreFrames()) {
            try {
                auto frameData = videoStream_->nextFrame();

                RTPPacketBuilder builder;
                auto packet = builder.setSequenceNumber(sequenceNumber_++)
                                     .setTimestamp(timestamp_)
                                     .setSSRC(ssrc_)
                                     .setPayload(frameData)
                                     .setMarker(1)
                                     .build();
                
                auto data = packet.getPacketVector();
                
                //Gửi UDP: Ép kiểu vector<uint8_t> sang const uint8_t*
                rtpSocket.sendTo(data.data(), data.size(), clientIP_, clientRTPPort_);
                
                timestamp_ += 3600;

            } catch (const std::exception& e) {
                LOG_ERROR("Streaming error: " + std::string(e.what()));
                break;
            }
        } else {
            videoStream_->rewind();
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
    if (it != request.headers.end()) return it->second == sessionId_;
    return false;
}

ServerWorker::ServerWorker(int clientId, std::unique_ptr<Socket> rtspSocket) : 
        clientId_(clientId),
        socket_(std::move(rtspSocket)),
        state_(State::INIT),
        running_(false),
        streaming_(false),
        sequenceNumber_(0),
        timestamp_(0)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(100000, 999999);
    ssrc_ = dis(gen);
    
    //Khởi tạo timer cho 25 FPS (40ms)
    frameTimer_ = std::make_unique<Timer>(40);
}

ServerWorker::~ServerWorker() 
{
    stop();
}

void ServerWorker::run() 
{
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

void ServerWorker::stop() 
{
    running_ = false;
    streaming_ = false;

    if (streamingThread_.joinable()) {
        streamingThread_.join();
    }

    if (socket_) {
        socket_->close();
    }
}
