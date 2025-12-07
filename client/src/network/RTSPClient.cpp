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
      serverRTPPort_(0),
      rtspUri_("") {
    // Socket will be created and connected when first request is sent
    std::cout << "[RTSPClient] Created client for " << serverIP << ":" << serverPort << std::endl;
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
    // Lazy connection: create and connect socket if needed
    if (!rtspSocket_) {
        std::cout << "[sendRtspRequest] Creating and connecting socket..." << std::endl;
        rtspSocket_ = std::make_unique<Socket>(SocketType::TCP);
        rtspSocket_->setTimeout(5000);  // 5000ms = 5 seconds for RTSP responses
        rtspSocket_->connect(serverIP_, serverPort_);
        rtspSocket_->setNoDelay(true);
        std::cout << "[sendRtspRequest] Connected successfully" << std::endl;
    }

    std::cout << "[sendRtspRequest] Sending " << request.size() << " bytes..." << std::endl;

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

    std::cout << "[sendRtspRequest] Request sent successfully, waiting for response..."
              << std::endl;

    std::string response;
    constexpr size_t BUF_SIZE = 4096;
    std::vector<uint8_t> buf(BUF_SIZE);

    try {
        while (true) {
            int received = rtspSocket_->receive(buf.data(), buf.size());
            std::cout << "[sendRtspRequest] Received " << received << " bytes" << std::endl;

            if (received < 0)
                throw SocketException("Failed to receive RTSP response");
            if (received == 0)
                break;  // connection closed

            response.append(reinterpret_cast<char*>(buf.data()), static_cast<size_t>(received));

            if (response.find("\r\n\r\n") != std::string::npos) {
                std::cout << "[sendRtspRequest] Found end of headers, response complete"
                          << std::endl;
                break;  // end of headers
            }
            if (static_cast<size_t>(received) < BUF_SIZE)
                break;
        }
    } catch (const SocketTimeout& e) {
        std::cerr << "[sendRtspRequest] ERROR: Socket timeout while waiting for response"
                  << std::endl;
        throw SocketException("RTSP response timeout");
    }

    std::cout << "[sendRtspRequest] Total response: " << response.size() << " bytes" << std::endl;
    return response;
}

int RTSPClient::parseStatusCode(const std::string& response) const {
    std::istringstream ss(response);
    std::string line;
    if (!std::getline(ss, line)) {
        std::cerr << "[parseStatusCode] ERROR: Cannot read first line from response" << std::endl;
        return 0;
    }

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::cout << "[parseStatusCode] First line: '" << line << "'" << std::endl;

    std::istringstream ls(line);
    std::string proto;
    int code = 0;
    ls >> proto >> code;

    std::cout << "[parseStatusCode] Protocol: '" << proto << "', Code: " << code << std::endl;

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

    // Construct proper RTSP URI
    std::string rtspUri =
        "rtsp://" + serverIP_ + ":" + std::to_string(serverPort_) + "/" + videoFile;

    std::ostringstream req;
    req << "SETUP " << rtspUri << " RTSP/1.0\r\n";
    req << "CSeq: " << ++cseq_ << "\r\n";
    req << "Transport: RTP/AVP/UDP;unicast;client_port=" << clientRTPPort << "-"
        << (clientRTPPort + 1) << "\r\n";
    req << "\r\n";  // Empty line to end headers

    std::cout << "=== SETUP Request ===\n" << req.str() << std::endl;
    std::cout << "Request length: " << req.str().size() << " bytes\n" << std::endl;

    std::string resp = sendRtspRequest(req.str());

    std::cout << "=== SETUP Response ===\n" << resp << std::endl;
    std::cout << "Response length: " << resp.size() << " bytes\n" << std::endl;

    int code = parseStatusCode(resp);
    std::cout << "Parsed status code: " << code << std::endl;
    if (code != 200) {
        std::cerr << "SETUP failed with code: " << code << std::endl;
        return false;
    }

    std::string sid = extractSessionId(resp);

    std::cout << "Session ID: " << sid << std::endl;

    if (sid.empty()) {
        std::cerr << "Failed to extract session info from response" << std::endl;
        return false;
    }

    // int srvPort = extractServerRTPPort(resp);
    // if (srvPort != 0) {
    //     serverRTPPort_ = srvPort;
    // }

    sessionId_ = sid;
    clientRTPPort_ = clientRTPPort;
    state_ = State::READY;
    rtspUri_ = videoFile;

    return true;
}

bool RTSPClient::sendPlay() {
    if (!validateState(State::READY))
        return false;
    if (sessionId_.empty())
        return false;

    // Construct proper RTSP URI
    std::string rtspUri =
        "rtsp://" + serverIP_ + ":" + std::to_string(serverPort_) + "/" + rtspUri_;

    std::ostringstream req;
    req << "PLAY " << rtspUri << " RTSP/1.0\r\n";
    req << "CSeq: " << ++cseq_ << "\r\n";
    req << "Session: " << sessionId_ << "\r\n";
    req << "Range: npt=0.000-\r\n";  // Start from beginning
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
    req << "PLAY " << rtspUri_ << " RTSP/1.0\r\n";
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
    req << "PLAY " << rtspUri_ << " RTSP/1.0\r\n";
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