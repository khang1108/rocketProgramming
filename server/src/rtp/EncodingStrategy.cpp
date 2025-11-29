/**
 * @file EncodingStrategy.cpp
 * @brief Implementation of RTP Encoding Strategies (Cross-platform:
 * Linux/Windows)
 *
 * @details
 * This file implements encoding strategies for RTP packet fragmentation.
 * All code uses standard C++ (STL) - no platform-specific code needed.
 *
 * Supports:
 * - Linux (GCC, Clang)
 * - Windows (MSVC, MinGW)
 * - Any platform with C++17 support
 */

#include "rtp/EncodingStrategy.hpp"
#include <algorithm>  // std::min
#include <stdexcept>  // std::runtime_error
#include <string>     // std::to_string

// ==================== Frame Implementation ====================

Frame::Frame(const std::vector<uint8_t>& d, uint16_t seq, uint32_t ts, uint32_t src)
    : data(d), sequenceNumber(seq), timestamp(ts), ssrc(src) {}

// ==================== SDEncodingStrategy Implementation ====================

std::vector<RTPPacket> SDEncodingStrategy::execute(const Frame& frame) {
    std::vector<RTPPacket> packets;

    // Build single packet
    RTPPacket packet = RTPPacketBuilder()
                           .setPayloadType(RTPPacket::MJPEG_TYPE)
                           .setSequenceNumber(frame.sequenceNumber)
                           .setTimestamp(frame.timestamp)
                           .setSSRC(frame.ssrc)
                           .setMarker(1)  // Single packet = complete frame, marker = 1
                           .setPayload(frame.data)
                           .build();

    packets.push_back(std::move(packet));
    return packets;
}

// ==================== HDEncodingStrategy Implementation ====================

std::vector<RTPPacket> HDEncodingStrategy::execute(const Frame& frame) {
    std::vector<RTPPacket> packets;

    const size_t frameSize = frame.data.size();
    const size_t maxPayload = RTPPacket::MAX_PAYLOAD_SIZE;
    size_t offset = 0;
    uint16_t currentSeq = frame.sequenceNumber;

    // Fragment frame into chunks
    while (offset < frameSize) {
        // Calculate chunk size
        size_t chunkSize = std::min(maxPayload, frameSize - offset);
        bool isLast = (offset + chunkSize >= frameSize);

        // Build packet for this fragment
        RTPPacket packet = RTPPacketBuilder()
                               .setPayloadType(RTPPacket::MJPEG_TYPE)
                               .setSequenceNumber(currentSeq++)
                               .setTimestamp(frame.timestamp)  // SAME timestamp for all fragments
                               .setSSRC(frame.ssrc)
                               .setMarker(isLast ? 1 : 0)  // Marker = 1 only for last fragment
                               .setPayload(&frame.data[offset], chunkSize)
                               .build();

        packets.push_back(std::move(packet));
        offset += chunkSize;
    }

    return packets;
}

// ==================== EncodingContext Implementation ====================

EncodingContext::EncodingContext()
    : Context<Frame, std::vector<RTPPacket>>(),
      sdStrategy_(std::make_unique<SDEncodingStrategy>()),
      hdStrategy_(std::make_unique<HDEncodingStrategy>()),
      autoDetect_(true) {}

void EncodingContext::setAutoDetect(bool enable) {
    autoDetect_ = enable;
}

std::vector<RTPPacket> EncodingContext::encodeFrame(const Frame& frame) {
    if (autoDetect_) {
        // Auto-select strategy based on frame size
        if (frame.data.size() <= RTPPacket::MAX_PAYLOAD_SIZE) {
            return sdStrategy_->execute(frame);
        } else {
            return hdStrategy_->execute(frame);
        }
    } else {
        // Use manually set strategy
        if (!hasStrategy()) {
            throw std::runtime_error("Strategy not set and auto-detect disabled");
        }
        return executeStrategy(frame);
    }
}

std::string EncodingContext::getEncodingInfo(const Frame& frame) const {
    size_t frameSize = frame.data.size();
    if (frameSize <= RTPPacket::MAX_PAYLOAD_SIZE) {
        return "SD Encoding (single packet): " + std::to_string(frameSize) + " bytes";
    } else {
        size_t numPackets =
            (frameSize + RTPPacket::MAX_PAYLOAD_SIZE - 1) / RTPPacket::MAX_PAYLOAD_SIZE;
        return "HD Encoding (" + std::to_string(numPackets) +
               " packets): " + std::to_string(frameSize) + " bytes";
    }
}
