#include "rtp/RTPReceiver.hpp"

#include <chrono>
#include <cstring>
#include <iostream>

#include "network/Socket.hpp"
#include "rtp/RTPPacket.hpp"

RTPReceiver::RTPReceiver(int rtpPort)
    : socket_(nullptr),
      rtpPort_(rtpPort),
      running_(false),
      packetReceived_(0),
      packetLost_(0),
      bytesReceived_(0),
      lastSequenceNumber_(0) {
    try {
        socket_ = std::make_unique<Socket>(SocketType::UDP);
        socket_->setReuseAddress(true);
        socket_->bind("0.0.0.0", rtpPort_);
        // Small timeout so receiveLoop can check running_ frequently
        socket_->setTimeout(5);  // milliseconds
    } catch (const SocketException& e) {
        // Propagate exception to caller
        throw;
    }
}

RTPReceiver::~RTPReceiver() {
    stop();
}

void RTPReceiver::start() {
    if (running_)
        return;
    running_ = true;
    receiverThread_ = std::thread(&RTPReceiver::receiveLoop, this);
}

void RTPReceiver::stop() {
    if (!running_)
        return;
    running_ = false;
    // Closing socket will cause receiveFrom to throw / unblock
    try {
        if (socket_)
            socket_->close();
    } catch (...) {
    }
    if (receiverThread_.joinable())
        receiverThread_.join();
}

void RTPReceiver::receiveLoop() {
    const size_t BUF_SIZE = 2048;
    std::vector<uint8_t> buffer(BUF_SIZE);

    while (running_) {
        try {
            std::string fromAddr;
            int fromPort = 0;
            int bytes = socket_->receiveFrom(buffer.data(), (int)buffer.size(), fromAddr, fromPort);
            if (bytes <= 0) {
                continue;
            }

            RTPPacket packet(buffer.data(), static_cast<size_t>(bytes));

            bytesReceived_ += static_cast<uint64_t>(bytes);
            packetReceived_++;

            updateStatistics(packet.getSequenceNumber());

            if (callback_) {
                try {
                    callback_(packet);
                } catch (const std::exception& e) {
                    // swallow callback exceptions to keep receiver running
                    std::cerr << "RTPReceiver callback exception: " << e.what() << std::endl;
                }
            }
        } catch (const SocketTimeout& e) {
            // Timeout - loop again to check running_
            continue;
        } catch (const SocketException& e) {
            // Socket error - break the loop
            std::cerr << "RTPReceiver socket error: " << e.what() << std::endl;
            break;
        } catch (const std::exception& e) {
            std::cerr << "RTPReceiver error: " << e.what() << std::endl;
            continue;
        }
    }
}

void RTPReceiver::updateStatistics(uint16_t currentSeq) {
    // First packet initialization
    if (lastSequenceNumber_ == 0) {
        lastSequenceNumber_ = currentSeq;
        return;
    }

    int32_t diff = RTPPacket::sequenceDifference(currentSeq, lastSequenceNumber_);
    if (diff > 1) {
        packetLost_ += static_cast<uint64_t>(diff - 1);
    }
    lastSequenceNumber_ = currentSeq;
}

double RTPReceiver::getPacketLossPercentage() const {
    uint64_t total = packetReceived_ + packetLost_;
    if (total == 0)
        return 0.0;
    return (static_cast<double>(packetLost_) * 100.0) / static_cast<double>(total);
}
