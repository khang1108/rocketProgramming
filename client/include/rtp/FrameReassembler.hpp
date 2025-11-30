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
class FrameReassembler {
  public:
    /**
     * @struct Frame
     * @brief Complete reassembled frame (JPEG data)
     */
    struct Frame {
        uint32_t timestamp;         ///< Timestamp of the frame
        std::vector<uint8_t> data;  ///< JPEG data of the frame
        uint16_t firstSeq;          ///< First sequence number
        uint16_t lastSeq;           ///< Last sequence number
        int packetCount;            ///< Number of packets in the frame
    };

    using FrameCallback = std::function<void(const Frame& frame)>;

  private:
    /**
     * @struct PacketBuffer
     * @brief Buffer for packets of one frame
     */
};

#endif