#include "network/RTSPClient.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std;

RTSPClient::RTSPClient(const std::string& serverIP, int serverPort)
    : state_(State::INIT),
      cseq_(0),
      sessionId_(),
      serverIP_(serverIP),
      serverPort_(serverPort),
      clientRTPPort_(0),
      serverRTPPort_(0) {
    // Create and connect RTSP TCP socket
    rtspSocket_ = std::make_unique<Socket>(SocketType::TCP);
    rtspSocket_->setTimeout(5000);  // 5000ms = 5 seconds for RTSP responses
    rtspSocket_->connect(serverIP_, serverPort_);
    rtspSocket_->setNoDelay(true);
}

RTSPClient::~RTSPClient() {
    try {
        if (state_ == State::PLAYING || state_ == State::READY) {
            try {
                sendTeardown();
            } catch (...) {
            }
        }
    } catch (...) {
    }

    if (rtspSocket_) {
        try {
            rtspSocket_->close();
        } catch (...) {
        }
        rtspSocket_.reset();
    }
}

std::string RTSPClient::sendRtspRequest(const std::string& request) {
    if (!rtspSocket_)
        throw SocketException("RTSP socket not initialized");

    // Send request bytes
    const uint8_t* data = reinterpret_cast<const uint8_t*>(request.data());
    size_t remaining = request.size();
    while (remaining > 0) {
        int sent = rtspSocket_->send(data, remaining);
        if (sent <= 0)
            throw SocketException("Failed to send RTSP request");
        remaining -= static_cast<size_t>(sent);
        data += sent;
    }

    std::string response;
    constexpr size_t BUF_SIZE = 4096;
    std::vector<uint8_t> buf(BUF_SIZE);
    while (true) {
        int received = rtspSocket_->receive(buf.data(), buf.size());
        if (received < 0)
            throw SocketException("Failed to receive RTSP response");
        if (received == 0)
            break;  // connection closed
        response.append(reinterpret_cast<char*>(buf.data()), static_cast<size_t>(received));
        if (response.find("\r\n\r\n") != std::string::npos)
            break;  // end of headers
        if (static_cast<size_t>(received) < BUF_SIZE)
            break;
    }
    return response;
}

int RTSPClient::parseStatusCode(const std::string& response) const {
    std::istringstream ss(response);
    std::string line;
    if (!std::getline(ss, line))
        return 0;

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::istringstream ls(line);
    std::string proto;
    int code = 0;
    ls >> proto >> code;
    return code;
}

std::string RTSPClient::extractSessionId(const std::string& response) const {
    std::istringstream ss(response);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find("Session:") != std::string::npos) {
            auto pos = line.find(":");
            if (pos != std::string::npos) {
                std::string val = line.substr(pos + 1);
                // trim
                while (!val.empty() && std::isspace((unsigned char)val.front()))
                    val.erase(val.begin());
                while (!val.empty() && std::isspace((unsigned char)val.back()))
                    val.pop_back();
                // strip possible ; parameters
                auto semi = val.find(';');
                if (semi != std::string::npos)
                    val = val.substr(0, semi);
                return val;
            }
        }
    }
    return std::string();
}

int RTSPClient::extractServerRTPPort(const std::string& response) const {
    std::istringstream ss(response);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find("Transport:") != std::string::npos) {
            // look for server_port=XXXX
            auto pos = line.find("server_port=");
            if (pos != std::string::npos) {
                pos += strlen("server_port=");
                std::string num;
                while (pos < line.size() && std::isdigit((unsigned char)line[pos])) {
                    num.push_back(line[pos]);
                    ++pos;
                }
                if (!num.empty())
                    return std::stoi(num);
            }
        }
    }
    return 0;
}

bool RTSPClient::validateState(State expectedState) const {
    return state_ == expectedState;
}

bool RTSPClient::sendSetup(const std::string& videoFile, int clientRTPPort) {
    if (!validateState(State::INIT))
        return false;

    std::ostringstream req;
    req << "SETUP " << videoFile << " RTSP/1.0\r\n";
    req << "CSeq: " << ++cseq_ << "\r\n";
    req << "Transport: RTP/UDP; client_port=" << clientRTPPort << "\r\n";
    req << "\r\n";

    std::string resp = sendRtspRequest(req.str());
    int code = parseStatusCode(resp);
    if (code != 200)  // 200 = OK
        return false;

    std::string sid = extractSessionId(resp);
    int srvPort = extractServerRTPPort(resp);
    if (sid.empty() || srvPort == 0)
        return false;

    sessionId_ = sid;
    clientRTPPort_ = clientRTPPort;
    serverRTPPort_ = srvPort;
    state_ = State::READY;
    return true;
}

bool RTSPClient::sendPlay() {
    if (!validateState(State::READY))
        return false;
    if (sessionId_.empty())
        return false;

    std::ostringstream req;
    req << "PLAY RTSP/1.0\r\n";
    req << "CSeq: " << ++cseq_ << "\r\n";
    req << "Session: " << sessionId_ << "\r\n";
    req << "\r\n";

    std::string resp = sendRtspRequest(req.str());
    int code = parseStatusCode(resp);
    if (code != 200)
        return false;

    state_ = State::PLAYING;
    return true;
}

bool RTSPClient::sendPause() {
    if (!validateState(State::PLAYING))
        return false;
    if (sessionId_.empty())
        return false;

    std::ostringstream req;
    req << "PAUSE RTSP/1.0\r\n";
    req << "CSeq: " << ++cseq_ << "\r\n";
    req << "Session: " << sessionId_ << "\r\n";
    req << "\r\n";

    std::string resp = sendRtspRequest(req.str());
    int code = parseStatusCode(resp);
    if (code != 200)
        return false;

    state_ = State::READY;
    return true;
}

bool RTSPClient::sendTeardown() {
    // TEARDOWN can be called from any state
    std::ostringstream req;
    req << "TEARDOWN RTSP/1.0\r\n";
    req << "CSeq: " << ++cseq_ << "\r\n";
    if (!sessionId_.empty())
        req << "Session: " << sessionId_ << "\r\n";
    req << "\r\n";

    try {
        std::string resp = sendRtspRequest(req.str());
        int code = parseStatusCode(resp);
        sessionId_.clear();
        state_ = State::INIT;
        clientRTPPort_ = 0;
        serverRTPPort_ = 0;
        return code == 200;
    } catch (const SocketException&) {
        sessionId_.clear();
        state_ = State::INIT;
        clientRTPPort_ = 0;
        serverRTPPort_ = 0;
        return false;
    }
}

std::string RTSPClient::getStateString() const {
    if (state_ == State::INIT)
        return "INIT";
    else if (state_ == State::READY)
        return "READY";
    else if (state_ == State::PLAYING)
        return "PLAYING";
    else
        return "UNKNOWN";
}
