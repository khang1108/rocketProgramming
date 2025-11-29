#ifndef CLIENT_RTP_FRAMEREASSEMBLER_HPP
#define CLIENT_RTP_FRAMEREASSEMBLER_HPP

#include <functional>
#include <map>
#include <mutex>
#include <vector>
#include "rtp/RTPPacket.hpp"

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
public:
    /**
    * @struct Frame
    * @brief Complete reassembled frame (JPEG data)
    */
    struct Frame
    {
        uint32_t timestamp; ///< Timestamp of the frame
        std::vector<uint8_t> data; ///< JPEG data of the frame
        uint16_t firstSeq; ///< First sequence number
        uint16_t lastSeq; ///< Last sequence number
        int packetCount; ///< Number of packets in the frame
    };

    using FrameCallback = std::function<void(const Frame& frame)>;

private:
    /**
    * @struct PacketBuffer
    * @brief Buffer for packets of one frame
    */
    struct PacketBuffer
    {
        uint32_t timestamp; ///< Timestamp of the frame
        std::map<uint16_t, std::vector<uint8_t>> packets; ///< seq -> payload
        bool complete;
        uint16_t lastSeq;
    };

    std::map<uint32_t, PacketBuffer> buffers_;
    std::mutex mutex_;

    FrameCallback callback_;

    /**
    * @brief Add packet to buffer
    * @param packet RTP packet
    */
    void addPacket(const RTPPacket& packet);

    /**
    * @brief Check if frame is complete and reassemble
    * @param timestamp Frame timestamp
    * @return True if reassembled and delivered
    */
    bool tryReassemble(uint32_t timestamp);

    /**
    * @brief Cleanup old incomplete frames
    */
    void cleanup();

public:
    FrameReassembler();

    /**
     * @brief Set frame callback
     * @param callback Function to call for complete frames
     * 
     * @example
     * reassembler.setCallback([&frameBuffer](const Frame& frame) {
     *     frameBuffer.push(frame.data);
     * });
     */
    void setCallback(FrameCallback callback);

    /**
     * @brief Process incoming RTP packet
     * @param packet RTP packet from receiver
     * 
     * @details
     * Algorithm:
     * 1. Add packet to buffer (keyed by timestamp)
     * 2. Sort packets by sequence number
     * 3. If marker bit set, try to reassemble
     * 4. If complete, concatenate payloads and call callback
     */
    void processPacket(const RTPPacket& packet);

    /**
    * @brief Get number of incomplete frames in buffer
    */
    size_t getPendingFrameCount() const;
};

#endif