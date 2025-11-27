#ifndef RTPPacket_SERVER_HPP
#define RTPPacket_SERVER_HPP

#include <array>
#include <vector>
#include <string>
#include <thread>
#include <memory>
#include <exception>

/**
 * @class RTPPacket
 * @brief Handles RTP (Real-time Transport Protocol) packet encoding and decoding for video streaming
 * 
 * @details
 * RTP packet structure (RFC 3550):
 * - Fixed 12-byte header containing:
 *   - Version (V, 2 bits): Always 2 for RTP
 *   - Padding (P, 1 bit): Indicates if padding bytes exist at the end
 *   - Extension (X, 1 bit): Indicates if header extension follows the fixed header
 *   - CSRC Count (CC, 4 bits): Number of CSRC identifiers (always 0 in this lab)
 *   - Marker (M, 1 bit): Marks significant events (e.g., end of frame fragment)
 *   - Payload Type (PT, 7 bits): 26 for MJPEG video
 *   - Sequence Number (16 bits): Increments by 1 for each packet sent
 *   - Timestamp (32 bits): Sampling instant of the first byte in the payload
 *   - SSRC (32 bits): Synchronization source identifier (server ID)
 * - Variable-length payload containing the video frame data
 * 
 * Header format:
 * <pre>
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           synchronization source (SSRC) identifier            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                            payload                            |
 * |                              ...                              |
 * </pre>
 * 
 * @note This class is used by both server (encoding) and client (decoding)
 * @note All multi-byte fields are in network byte order (big-endian)
 */
class RTPPacket
{
public:
    // RTP Constants
    static constexpr int HEADER_SIZE = 12;           ///< Fixed header size in bytes
    static constexpr int RTP_VERSION = 2;            ///< RTP protocol version
    static constexpr int MJPEG_TYPE = 26;            ///< Payload type for MJPEG video
    static constexpr int MAX_PAYLOAD_SIZE = 1400;    ///< Maximum payload size for MTU consideration

private:
    // RTP Header Fields (parsed values)
    uint8_t version_;          ///< RTP version (2 bits)
    uint8_t padding_;          ///< Padding flag (1 bit)
    uint8_t extension_;        ///< Extension flag (1 bit)
    uint8_t cc_;               ///< CSRC count (4 bits)
    uint8_t marker_;           ///< Marker bit (1 bit) - set to 1 for last fragment
    uint8_t payloadType_;      ///< Payload type (7 bits) - 26 for MJPEG
    uint16_t sequenceNumber_;  ///< Sequence number (16 bits)
    uint32_t timestamp_;       ///< Timestamp (32 bits)
    uint32_t ssrc_;            ///< SSRC identifier (32 bits)

    std::array<uint8_t, HEADER_SIZE> header; // Fixed 12-byte header
    std::vector<uint8_t> payload;
public:
    RTPPacket(){
        header.fill(0);
    }

    /**
     * @brief Constructor for encoding (SERVER SIDE)
     * @details Creates an RTP packet from header fields and payload data
     * 
     * @param payloadType Payload type (26 for MJPEG)
     * @param sequenceNumber Packet sequence number (frame number)
     * @param timestamp Timestamp for this frame (milliseconds since epoch)
     * @param ssrc Synchronization source identifier (server ID)
     * @param marker Marker bit (1 for last fragment, 0 otherwise)
     * @param data Pointer to payload data (video frame bytes)
     * @param dataLength Length of payload data in bytes
     * 
     * @throws std::invalid_argument if data is nullptr or dataLength is 0
     * @throws std::length_error if dataLength exceeds reasonable limits
     * 
     * @note This automatically calls encode() to build the header
     * 
     * @example
     * @code
     * std::vector<uint8_t> frameData = videoStream.getNextFrame();
     * RTPPacket packet(26, frameNum, getCurrentTime(), 12345, 0, 
     *                  frameData.data(), frameData.size());
     * @endcode
     */
    RTPPacket(uint8_t payloadType, uint16_t sequenceNumber, uint32_t timestamp,
            uint32_t ssrc, uint8_t marker, const uint8_t* data, size_t dataLength);

    /**
     * @brief Constructor for decoding (CLIENT SIDE)
     * @details Parses an RTP packet from raw byte stream received over network
     * 
     * @param packet Pointer to raw packet bytes (header + payload)
     * @param packetSize Total size of packet in bytes
     * 
     * @throws std::invalid_argument if packet is nullptr
     * @throws std::length_error if packetSize < HEADER_SIZE
     * 
     * @note This automatically calls decode() to parse the header
     * 
     * @example
     * @code
     * uint8_t buffer[2000];
     * int bytesReceived = socket.receiveFrom(buffer, sizeof(buffer));
     * RTPPacket packet(buffer, bytesReceived);
     * @endcode
     */
    RTPPacket(const uint8_t* packet, size_t packetSize);

    //* Setters
    void setHeader(const uint8_t *h);
    void setPayload(const std::vector<uint8_t> &p);
    void setPayload(const uint8_t *data, size_t len);

    //* Getters

};
#endif