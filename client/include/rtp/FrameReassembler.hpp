#ifndef CLIENT_RTP_FRAMEREASSEMBLER_HPP
#define CLIENT_RTP_FRAMEREASSEMBLER_HPP

#include <map>
#include <mutex>
#include <vector>
#include "rtp/RTPPacket.hpp"

class FrameBuffer;

/**
 * @class FrameReassembler
 * @brief Reassembles fragmented RTP packets into complete frames
 * 
 * Handles:
 * - Out-of-order packets (sorts by sequence number)
 * - Frame boundary detection (marker bit)
 * - Missing packets (timeout)
 */
class FrameReassembler
{
private:
    FrameBuffer* frameBuffer_;

    std::map<uint32_t, std::vector<RTPPacket>> frameBuffers_;
    mutable std::mutex mutex_;

    void reassembleFrame(uint32_t timestamp);

public:
        /**
     * @brief Constructor
     * @param frameBuffer Pointer to FrameBuffer (does not own)
     */
    explicit FrameReassembler(FrameBuffer* frameBuffer);

    /**
     * @brief Add RTP packet for reassembly
     * @param packet RTP packet from receiver
     * 
     * @details
     * Algorithm:
     * 1. Add packet to buffer (keyed by timestamp)
     * 2. If marker bit set, reassemble frame
     * 3. Sort packets by sequence number
     * 4. Concatenate payloads and push to FrameBuffer
     */
    void addPacket(const RTPPacket& packet);

    /**
    * @brief Get number of incomplete frames in buffer
    */
    size_t getPendingFrameCount() const;
};

#endif