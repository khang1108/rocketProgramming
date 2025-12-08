#include "rtp/FrameReassembler.hpp"
#include <algorithm>
#include "buffer/FrameBuffer.hpp"
#include "utils/Logger.hpp"

FrameReassembler::FrameReassembler(FrameBuffer* frameBuffer) : frameBuffer_(frameBuffer) {
    if (!frameBuffer_) {
        throw std::invalid_argument("FrameBuffer cannot be null");
    }
}

void FrameReassembler::addPacket(const RTPPacket& packet) {
    uint32_t timestamp = packet.getTimestamp();
    bool shouldReassemble = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Detect video loop (timestamp jump backwards or very large jump forward)
        // Khi server rewind, timestamp có thể nhảy lớn hoặc quay về nhỏ
        if (!frameBuffers_.empty()) {
            uint32_t lastTs = frameBuffers_.rbegin()->first;
            int64_t diff = static_cast<int64_t>(timestamp) - static_cast<int64_t>(lastTs);

            // Nếu timestamp nhảy lùi hoặc nhảy quá xa (> 10 giây = 360000 units @ 90kHz)
            // thì clear pending frames cũ
            if (diff < -36000 || diff > 360000) {
                Logger::getInstance().log(
                    LogLevel::INFO,
                    "Timestamp jump detected (video loop?): " + std::to_string(lastTs) + " -> " +
                        std::to_string(timestamp) + ", clearing " +
                        std::to_string(frameBuffers_.size()) + " pending frames");
                frameBuffers_.clear();
            }
        }

        frameBuffers_[timestamp].push_back(packet);

        if (packet.getMarker()) {
            shouldReassemble = true;
        }

        const size_t MAX_PENDING = 64;
        if (frameBuffers_.size() > MAX_PENDING) {
            auto it = frameBuffers_.begin();
            Logger::getInstance().log(LogLevel::WARN, "Dropping incomplete frame: timestamp=" +
                                                          std::to_string(it->first));
            frameBuffers_.erase(it);
        }
    }

    // Reassemble NGOÀI mutex lock để tránh deadlock khi push blocking
    if (shouldReassemble) {
        reassembleFrame(timestamp);
    }
}

void FrameReassembler::reassembleFrame(uint32_t timestamp) {
    std::vector<uint8_t> frameData;
    size_t packetCount = 0;

    // Lock chỉ khi truy cập frameBuffers_
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = frameBuffers_.find(timestamp);
        if (it == frameBuffers_.end()) {
            return;
        }

        auto& packets = it->second;

        if (packets.empty()) {
            frameBuffers_.erase(it);
            return;
        }

        std::sort(packets.begin(), packets.end(), [](const RTPPacket& a, const RTPPacket& b) {
            return a.getSequenceNumber() < b.getSequenceNumber();
        });

        for (const auto& pkt : packets) {
            const auto& payload = pkt.getPayload();
            frameData.insert(frameData.end(), payload.begin(), payload.end());
        }

        packetCount = packets.size();
        frameBuffers_.erase(it);
    }

    // Push NGOÀI mutex lock - có thể block nhưng không chặn các packets khác
    bool success = frameBuffer_->push(frameData);

    if (success) {
        Logger::getInstance().log(LogLevel::INFO,
                                  "Frame reassembler: timestamp=" + std::to_string(timestamp) +
                                      ", packets=" + std::to_string(packetCount) +
                                      ", size=" + std::to_string(frameData.size()) + " bytes");
    } else {
        Logger::getInstance().log(LogLevel::WARN, "Failed to push frame (buffer closed?)");
    }
}

size_t FrameReassembler::getPendingFrameCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frameBuffers_.size();
}