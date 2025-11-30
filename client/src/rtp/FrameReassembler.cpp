#include "rtp/FrameReassembler.hpp"

#include "rtp/RTPPacket.hpp"

#include <chrono>
#include <cstdint>
#include <limits>

FrameReassembler::FrameReassembler() {}

void FrameReassembler::setCallback(FrameCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void FrameReassembler::processPacket(const RTPPacket& packet) {
    // Add packet to buffer and if marker bit is set attempt reassembly
    {
        std::lock_guard<std::mutex> lock(mutex_);
        addPacket(packet);
    }

    if (packet.getMarker()) {
        // Try to reassemble the frame for this timestamp
        if (tryReassemble(packet.getTimestamp()))
            return;
    }

    // Periodic cleanup to avoid unbounded memory growth
    cleanup();
}

void FrameReassembler::addPacket(const RTPPacket& packet) {
    // caller must hold lock when necessary; but safe to lock here as well
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t ts = packet.getTimestamp();
    auto it = buffers_.find(ts);
    if (it == buffers_.end()) {
        PacketBuffer pb;
        pb.timestamp = ts;
        pb.complete = false;
        pb.lastSeq = 0;
        auto res = buffers_.emplace(ts, std::move(pb));
        it = res.first;
    }

    auto& buf = it->second;
    buf.packets[packet.getSequenceNumber()] = packet.getPayload();

    if (packet.getMarker()) {
        buf.lastSeq = packet.getSequenceNumber();
    }
}

bool FrameReassembler::tryReassemble(uint32_t timestamp) {
    Frame frameOut;
    FrameCallback cb;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = buffers_.find(timestamp);
        if (it == buffers_.end())
            return false;

        PacketBuffer& buf = it->second;
        if (buf.packets.empty())
            return false;

        bool hasLastSeq = true;

        if (buf.lastSeq == 0) {
            if (!(buf.packets.size() == 1 && buf.packets.begin()->first == 0))
                hasLastSeq = false;
        }

        if (!hasLastSeq)
            return false;

        size_t packetCount = buf.packets.size();
        uint16_t chosenFirst = 0;
        bool foundFirst = false;
        for (const auto& kv : buf.packets) {
            uint16_t candidate = kv.first;
            int32_t diff = RTPPacket::sequenceDifference(buf.lastSeq, candidate);
            if (diff < 0)
                diff += 0x10000;  // wrap adjustment
            if (static_cast<size_t>(diff) + 1 == packetCount) {
                chosenFirst = candidate;
                foundFirst = true;
                break;
            }
        }

        if (!foundFirst)
            return false;

        // Verify all sequence numbers from chosenFirst to lastSeq exist
        std::vector<uint8_t> assembled;
        uint16_t cur = chosenFirst;
        while (true) {
            auto pit = buf.packets.find(cur);
            if (pit == buf.packets.end())
                return false;  // missing packet
            const std::vector<uint8_t>& payload = pit->second;
            assembled.insert(assembled.end(), payload.begin(), payload.end());
            if (cur == buf.lastSeq)
                break;
            cur = static_cast<uint16_t>(cur + 1);
        }

        // Fill frameOut fields
        frameOut.timestamp = buf.timestamp;
        frameOut.data = std::move(assembled);
        frameOut.firstSeq = chosenFirst;
        frameOut.lastSeq = buf.lastSeq;
        frameOut.packetCount = static_cast<int>(packetCount);

        // Erase buffer before invoking callback
        buffers_.erase(it);

        cb = callback_;  // copy callback under lock
    }

    if (cb) {
        cb(frameOut);
        return true;
    }
    return false;
}

void FrameReassembler::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t MAX_PENDING = 64;
    if (buffers_.size() <= MAX_PENDING)
        return;

    while (buffers_.size() > MAX_PENDING) {
        auto it = buffers_.begin();
        if (it == buffers_.end())
            break;
        buffers_.erase(it);
    }
}

size_t FrameReassembler::getPendingFrameCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return buffers_.size();
}
