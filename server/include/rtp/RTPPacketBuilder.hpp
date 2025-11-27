#ifndef RTPPACKETBUILDER_HPP
#define RTPPACKETBUILDER_HPP

#include "RTPPacket.hpp"
#include "../patterns/Builder.hpp"
#include <memory>
#include <stdexcept>

/**
 * @class RTPPacketBuilder
 * @brief Builder Pattern implementation để xây dựng RTP packets
 *
 * @details
 * RTPPacketBuilder sử dụng Builder Design Pattern để:
 * 1. Tách biệt logic xây dựng packet khỏi class RTPPacket
 * 2. Cung cấp fluent interface (method chaining) để dễ sử dụng
 * 3. Đảm bảo packet được xây dựng đúng với tất cả trường bắt buộc
 * 4. Validate dữ liệu trước khi build packet final
 *
 * Builder pattern đặc biệt hữu ích cho RTP packet vì:
 * - RTP header có nhiều trường phức tạp
 * - Cần đảm bảo các trường được set theo đúng thứ tự và định dạng
 * - Có thể có các variant khác nhau (SD vs HD, fragmented vs single)
 *
 * @example Sử dụng cơ bản (SD video, single packet):
 * @code
 * std::vector<uint8_t> frameData = videoStream->nextFrame();
 *
 * RTPPacket packet = RTPPacketBuilder()
 *     .setPayloadType(RTPPacket::MJPEG_TYPE)
 *     .setSequenceNumber(frameNumber++)
 *     .setTimestamp(RTPPacket::getCurrentTimestamp())
 *     .setSSRC(12345)  // Server ID
 *     .setMarker(0)    // Not last fragment
 *     .setPayload(frameData.data(), frameData.size())
 *     .build();
 *
 * // Send packet over UDP
 * socket->sendTo(packet.getPacketVector().data(), packet.getLength(), clientAddr);
 * @endcode
 *
 * @example Sử dụng với HD video fragmentation:
 * @code
 * // Large frame cần fragment
 * std::vector<uint8_t> largeFrame = videoStream->nextFrame();
 * size_t offset = 0;
 * uint16_t seqNum = currentSeq;
 *
 * while (offset < largeFrame.size()) {
 *     size_t chunkSize = std::min(RTPPacket::MAX_PAYLOAD_SIZE,
 *                                  largeFrame.size() - offset);
 *     bool isLast = (offset + chunkSize >= largeFrame.size());
 *
 *     RTPPacket packet = RTPPacketBuilder()
 *         .setPayloadType(RTPPacket::MJPEG_TYPE)
 *         .setSequenceNumber(seqNum++)
 *         .setTimestamp(timestamp)  // Same timestamp cho tất cả fragments
 *         .setSSRC(serverID)
 *         .setMarker(isLast ? 1 : 0)  // Marker=1 cho fragment cuối
 *         .setPayload(&largeFrame[offset], chunkSize)
 *         .build();
 *
 *     socket->sendTo(packet.getPacketVector().data(), packet.getLength(), clientAddr);
 *     offset += chunkSize;
 * }
 * @endcode
 *
 * @see Builder - Base builder pattern interface
 * @see RTPPacket - Data class được build bởi builder này
 * @see EncodingStrategy - Strategy pattern cho SD/HD encoding
 */
class RTPPacketBuilder : public Builder<RTPPacket>
{
private:
    // Header fields
    uint8_t version_;
    uint8_t padding_;
    uint8_t extension_;
    uint8_t cc_;
    uint8_t marker_;
    uint8_t payloadType_;
    uint16_t sequenceNumber_;
    uint32_t timestamp_;
    uint32_t ssrc_;

    // Payload
    std::vector<uint8_t> payload_;

    // Validation flags
    bool seqSet_;
    bool timestampSet_;
    bool ssrcSet_;
    bool payloadSet_;

public:
    /**
     * @brief Constructor - khởi tạo builder với default values
     * @details Sets reasonable defaults:
     * - version = 2 (RTP standard)
     * - padding = 0 (no padding)
     * - extension = 0 (no extension header)
     * - cc = 0 (no CSRC)
     * - marker = 0 (not last fragment)
     * - payloadType = 26 (MJPEG)
     * - Other fields = 0 (must be set explicitly)
     */
    RTPPacketBuilder()
        : version_(RTPPacket::RTP_VERSION),
          padding_(0),
          extension_(0),
          cc_(0),
          marker_(0),
          payloadType_(RTPPacket::MJPEG_TYPE),
          sequenceNumber_(0),
          timestamp_(0),
          ssrc_(0),
          seqSet_(false),
          timestampSet_(false),
          ssrcSet_(false),
          payloadSet_(false) {}

    /**
     * @brief Set RTP version
     * @param version RTP version (must be 2)
     * @return Reference to this builder (for method chaining)
     * @throws std::invalid_argument if version != 2
     */
    RTPPacketBuilder &setVersion(uint8_t version)
    {
        if (version != 2)
        {
            throw std::invalid_argument("RTP version must be 2");
        }
        version_ = version;
        return *this;
    }

    /**
     * @brief Set padding flag
     * @param padding 1 if padding present, 0 otherwise
     * @return Reference to this builder
     */
    RTPPacketBuilder &setPadding(uint8_t padding)
    {
        padding_ = padding & 0x01;
        return *this;
    }

    /**
     * @brief Set extension flag
     * @param extension 1 if extension header present, 0 otherwise
     * @return Reference to this builder
     */
    RTPPacketBuilder &setExtension(uint8_t extension)
    {
        extension_ = extension & 0x01;
        return *this;
    }

    /**
     * @brief Set CSRC count
     * @param cc Number of CSRC identifiers (0-15)
     * @return Reference to this builder
     * @throws std::invalid_argument if cc > 15
     */
    RTPPacketBuilder &setCC(uint8_t cc)
    {
        if (cc > 15)
        {
            throw std::invalid_argument("CC must be 0-15");
        }
        cc_ = cc;
        return *this;
    }

    /**
     * @brief Set marker bit
     * @param marker 1 for last fragment of frame, 0 otherwise
     * @return Reference to this builder
     * @note Quan trọng cho HD fragmentation - phải set = 1 cho packet cuối
     */
    RTPPacketBuilder &setMarker(uint8_t marker)
    {
        marker_ = marker & 0x01;
        return *this;
    }

    /**
     * @brief Set payload type
     * @param payloadType Payload type code (26 for MJPEG)
     * @return Reference to this builder
     * @throws std::invalid_argument if payloadType > 127
     */
    RTPPacketBuilder &setPayloadType(uint8_t payloadType)
    {
        if (payloadType > 127)
        {
            throw std::invalid_argument("Payload type must be 0-127");
        }
        payloadType_ = payloadType;
        return *this;
    }

    /**
     * @brief Set sequence number (REQUIRED)
     * @param sequenceNumber Packet sequence number (0-65535)
     * @return Reference to this builder
     * @note Sequence number wraps at 65535, phải increment cho mỗi packet
     */
    RTPPacketBuilder &setSequenceNumber(uint16_t sequenceNumber)
    {
        sequenceNumber_ = sequenceNumber;
        seqSet_ = true;
        return *this;
    }

    /**
     * @brief Set timestamp (REQUIRED)
     * @param timestamp Timestamp in RTP time units
     * @return Reference to this builder
     * @note Dùng RTPPacket::getCurrentTimestamp() để lấy timestamp hiện tại
     * @note Tất cả fragments của cùng frame phải có SAME timestamp
     */
    RTPPacketBuilder &setTimestamp(uint32_t timestamp)
    {
        timestamp_ = timestamp;
        timestampSet_ = true;
        return *this;
    }

    /**
     * @brief Set SSRC identifier (REQUIRED)
     * @param ssrc Synchronization source identifier (server ID)
     * @return Reference to this builder
     * @note Mỗi server/stream phải có unique SSRC
     */
    RTPPacketBuilder &setSSRC(uint32_t ssrc)
    {
        ssrc_ = ssrc;
        ssrcSet_ = true;
        return *this;
    }

    /**
     * @brief Set payload data from raw bytes (REQUIRED)
     * @param data Pointer to payload data
     * @param length Size of payload in bytes
     * @return Reference to this builder
     * @throws std::invalid_argument if data is nullptr or length is 0
     * @warning Payload size không nên vượt quá MAX_PAYLOAD_SIZE (1400 bytes)
     */
    RTPPacketBuilder &setPayload(const uint8_t *data, size_t length)
    {
        if (!data || length == 0)
        {
            throw std::invalid_argument("Invalid payload data");
        }
        payload_.assign(data, data + length);
        payloadSet_ = true;
        return *this;
    }

    /**
     * @brief Set payload data from vector
     * @param data Payload data vector
     * @return Reference to this builder
     * @throws std::invalid_argument if data is empty
     */
    RTPPacketBuilder &setPayload(const std::vector<uint8_t> &data)
    {
        if (data.empty())
        {
            throw std::invalid_argument("Payload cannot be empty");
        }
        payload_ = data;
        payloadSet_ = true;
        return *this;
    }

    /**
     * @brief Build final RTP packet
     * @return Constructed RTPPacket
     * @throws std::runtime_error if required fields not set
     *
     * @details
     * Build process:
     * 1. Validate tất cả required fields đã được set
     * 2. Tạo RTPPacket object
     * 3. Set tất cả header fields
     * 4. Set payload
     * 5. Gọi encode() để build raw header bytes
     * 6. Trả về packet hoàn chỉnh
     */
    RTPPacket build() override
    {
        // Validate required fields
        if (!seqSet_)
        {
            throw std::runtime_error("Sequence number not set");
        }
        if (!timestampSet_)
        {
            throw std::runtime_error("Timestamp not set");
        }
        if (!ssrcSet_)
        {
            throw std::runtime_error("SSRC not set");
        }
        if (!payloadSet_)
        {
            throw std::runtime_error("Payload not set");
        }

        // Create packet
        RTPPacket packet;

        // Set header fields
        packet.setVersion(version_);
        packet.setPadding(padding_);
        packet.setExtension(extension_);
        packet.setCC(cc_);
        packet.setMarker(marker_);
        packet.setPayloadType(payloadType_);
        packet.setSequenceNumber(sequenceNumber_);
        packet.setTimestamp(timestamp_);
        packet.setSSRC(ssrc_);

        // Set payload
        packet.setPayload(payload_);

        // Encode header into raw bytes
        packet.encode();

        return packet;
    }

    /**
     * @brief Reset builder về trạng thái ban đầu
     * @note Useful để reuse builder cho nhiều packets
     */
    void reset() override
    {
        version_ = RTPPacket::RTP_VERSION;
        padding_ = 0;
        extension_ = 0;
        cc_ = 0;
        marker_ = 0;
        payloadType_ = RTPPacket::MJPEG_TYPE;
        sequenceNumber_ = 0;
        timestamp_ = 0;
        ssrc_ = 0;
        payload_.clear();
        seqSet_ = false;
        timestampSet_ = false;
        ssrcSet_ = false;
        payloadSet_ = false;
    }

    /**
     * @brief Check if all required fields are set
     * @return true if builder ready to build
     */
    bool isReady() const
    {
        return seqSet_ && timestampSet_ && ssrcSet_ && payloadSet_;
    }
};

#endif // RTPPACKETBUILDER_HPP
