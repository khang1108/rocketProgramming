#ifndef SERVER_RTP_RTPPACKET_HPP
#define SERVER_RTP_RTPPACKET_HPP

#include <array>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

/**
 * @class RTPPacket
 * @brief Handles RTP (Real-time Transport Protocol) packet encoding and decoding for video
 * streaming
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
class RTPPacket {
  public:
    // RTP Constants
    static constexpr int HEADER_SIZE = 12;         ///< Fixed header size in bytes
    static constexpr int RTP_VERSION = 2;          ///< RTP protocol version
    static constexpr int MJPEG_TYPE = 26;          ///< Payload type for MJPEG video
    static constexpr int MAX_PAYLOAD_SIZE = 1400;  ///< Maximum payload size for MTU consideration

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

    // Raw packet data (network byte order)
    std::array<uint8_t, HEADER_SIZE> header_;  // Fixed 12-byte header
    std::vector<uint8_t> payload_;             // Payload data

  public:
    // ==================== CONSTRUCTORS ====================

    /**
     * @brief Default constructor - creates empty packet
     */
    RTPPacket()
        : version_(RTP_VERSION),
          padding_(0),
          extension_(0),
          cc_(0),
          marker_(0),
          payloadType_(MJPEG_TYPE),
          sequenceNumber_(0),
          timestamp_(0),
          ssrc_(0) {
        header_.fill(0);
    }

    /**
     * @brief Constructor for decoding (CLIENT SIDE)
     * @param packet Raw packet bytes
     * @param packetSize Total packet size
     * @throws std::invalid_argument if packet is nullptr
     * @throws std::length_error if packetSize < HEADER_SIZE
     */
    RTPPacket(const uint8_t* packet, size_t packetSize) : RTPPacket() {
        decode(packet, packetSize);
    }

    // ==================== ENCODING/DECODING ====================

    /**
     * @brief Encode parsed fields into raw header bytes
     * @details Packs header fields into 12-byte array (network byte order)
     * @note Called automatically by RTPPacketBuilder
     */
    void encode();

    /**
     * @brief Decode raw packet bytes into fields
     * @param packet Raw packet bytes (header + payload)
     * @param packetSize Total packet size
     * @throws std::invalid_argument if packet is nullptr
     * @throws std::length_error if packetSize < HEADER_SIZE
     */
    void decode(const uint8_t* packet, size_t packetSize);

    // ==================== GETTERS ====================

    uint8_t getVersion() const { return version_; }
    uint8_t getPadding() const { return padding_; }
    uint8_t getExtension() const { return extension_; }
    uint8_t getCC() const { return cc_; }
    uint8_t getMarker() const { return marker_; }
    uint8_t getPayloadType() const { return payloadType_; }
    uint16_t getSequenceNumber() const { return sequenceNumber_; }
    uint32_t getTimestamp() const { return timestamp_; }
    uint32_t getSSRC() const { return ssrc_; }

    const std::vector<uint8_t>& getPayload() const { return payload_; }
    std::vector<uint8_t>& getPayload() { return payload_; }
    size_t getPayloadSize() const { return payload_.size(); }

    const std::array<uint8_t, HEADER_SIZE>& getHeader() const { return header_; }
    size_t getLength() const { return HEADER_SIZE + payload_.size(); }

    /**
     * @brief Get complete packet (header + payload) as contiguous bytes
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     * @return Number of bytes written
     * @throws std::length_error if buffer too small
     */
    size_t getPacket(uint8_t* buffer, size_t bufferSize) const;

    /**
     * @brief Get complete packet as vector
     * @return Vector containing header + payload
     */
    std::vector<uint8_t> getPacketVector() const;

    // ==================== SETTERS ====================

    void setVersion(uint8_t v);
    void setPadding(uint8_t p) { padding_ = p & 0x01; }
    void setExtension(uint8_t x) { extension_ = x & 0x01; }
    void setCC(uint8_t cc);
    void setMarker(uint8_t m) { marker_ = m & 0x01; }
    void setPayloadType(uint8_t pt);
    void setSequenceNumber(uint16_t seq) { sequenceNumber_ = seq; }
    void setTimestamp(uint32_t ts) { timestamp_ = ts; }
    void setSSRC(uint32_t ssrc) { ssrc_ = ssrc; }

    void setPayload(const uint8_t* data, size_t length);
    void setPayload(const std::vector<uint8_t>& data) { payload_ = data; }

    // ==================== UTILITY METHODS ====================

    /**
     * @brief Validate packet fields
     * @return true if all fields valid
     */
    bool validate() const;

    /**
     * @brief Print header for debugging
     */
    void printHeader() const;

    /**
     * @brief Get current timestamp in milliseconds
     * @return Current time since epoch
     */
    static uint32_t getCurrentTimestamp();

    /**
     * @brief Calculate sequence number difference with wrap-around
     * @param seq1 First sequence number
     * @param seq2 Second sequence number
     * @return Signed difference (handles 16-bit wrap at 65535)
     */
    static int32_t sequenceDifference(uint16_t seq1, uint16_t seq2);
};
#endif