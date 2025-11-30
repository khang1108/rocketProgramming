#include "rtp/FrameReassembler.hpp"
#include "buffer/FrameBuffer.hpp"
#include "utils/Logger.hpp"
#include <algorithm>

FrameReassembler::FrameReassembler(FrameBuffer* frameBuffer)
        : frameBuffer_(frameBuffer){
    if(!frameBuffer_){
        throw std::invalid_argument("FrameBuffer cannot be null");
    }
}

void FrameReassembler::addPacket(const RTPPacket& packet)
{
    std::lock_guard<std::mutex> lock(mutex_);

    uint32_t timestamp = packet.getTimestamp();

    frameBuffers_[timestamp].push_back(packet);

    if(packet.getMarker()){
        reassembleFrame(timestamp);
    }

    const size_t MAX_PENDING = 64;
    if(frameBuffers_.size() > MAX_PENDING){
        auto it = frameBuffers_.begin();
        Logger::getInstance().log(LogLevel::WARN, 
            "Dropping incomplete frame: timestamp=" + std::to_string(it->first));
        frameBuffers_.erase(it);
    }
}

void FrameReassembler::reassembleFrame(uint32_t timestamp)
{
    auto it = frameBuffers_.find(timestamp);
    if(it == frameBuffers_.end()){
        return;
    }

    auto &packets = it->second;

    if(packets.empty()){
        frameBuffers_.erase(it);
        return;
    }

    std::sort(packets.begin(), packets.end(), [](const RTPPacket& a, const RTPPacket& b){
        return a.getSequenceNumber() < b.getSequenceNumber();
    });

    std::vector<uint8_t> frameData;
    for(const auto& pkt: packets){
        const auto& payload = pkt.getPayload();
        frameData.insert(frameData.end(), payload.begin(),  payload.end());
    }

    bool success = frameBuffer_->push(frameData);

    if(success){
        Logger::getInstance().log(LogLevel::INFO, "Frame reassembler: timestamp=" + std::to_string(timestamp) +
                                    ", packets=" + std::to_string(packets.size()) + 
                                    ", size=" + std::to_string(frameData.size()) + " bytes");
    }else{
        Logger::getInstance().log(LogLevel::WARN, "Failed to push frame to buffer (buffer closed?)");
    }

    frameBuffers_.erase(it);
}

size_t FrameReassembler::getPendingFrameCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frameBuffers_.size();
}