#ifndef SERVER_RTP_ENCODINGSTRATEGY_HPP
#define SERVER_RTP_ENCODINGSTRATEGY_HPP

#include "../../common/include/patterns/Strategy.hpp"
#include "RTPPacket.hpp"
#include "RTPPacketBuilder.hpp"
#include <memory>
#include <string>
#include <vector>

/**
 * @struct Frame
 * @brief Đại diện cho một video frame cần encode
 */
struct Frame {
    std::vector<uint8_t> data; ///< Frame data (JPEG bytes)
    uint16_t sequenceNumber;   ///< Starting sequence number for this frame
    uint32_t timestamp;        ///< Frame timestamp
    uint32_t ssrc;             ///< Server SSRC

    Frame(const std::vector<uint8_t> &d, uint16_t seq, uint32_t ts, uint32_t src);
};

/**
 * @class EncodingStrategy
 * @brief Strategy Pattern base class cho các thuật toán encoding RTP packets
 *
 * @details
 * EncodingStrategy định nghĩa interface cho các chiến lược encoding khác nhau:
 * - **SDEncodingStrategy**: Cho SD video (frame < MTU) - một frame = một packet
 * - **HDEncodingStrategy**: Cho HD video (frame > MTU) - fragmentation cần
 * thiết
 *
 * @see SDEncodingStrategy - Concrete strategy cho SD video
 * @see HDEncodingStrategy - Concrete strategy cho HD video
 * @see EncodingContext - Context class sử dụng strategies
 */
using EncodingStrategy = Strategy<Frame, std::vector<RTPPacket>>;

/**
 * @class SDEncodingStrategy
 * @brief Strategy cho SD video - single packet per frame
 *
 * @details
 * Được sử dụng khi frame size ≤ MAX_PAYLOAD_SIZE (1400 bytes).
 * Đơn giản nhất: mỗi frame encode thành đúng 1 RTP packet.
 */
class SDEncodingStrategy : public EncodingStrategy {
public:
    /**
    * @brief Encode một frame thành một RTP packet
    * @param frame Frame cần encode
    * @return Vector chứa 1 RTP packet
    */
    std::vector<RTPPacket> execute(const Frame &frame) override;
};

/**
 * @class HDEncodingStrategy
 * @brief Strategy cho HD video - fragmentation into multiple packets
 *
 * @details
 * Được sử dụng khi frame size > MAX_PAYLOAD_SIZE (1400 bytes).
 * Fragment frame thành nhiều packets, mỗi packet ≤ MTU.
 */
class HDEncodingStrategy : public EncodingStrategy {
public:
    /**
    * @brief Encode một large frame thành multiple RTP packets
    * @param frame Frame cần encode (frame.data.size() > MAX_PAYLOAD_SIZE)
    * @return Vector chứa fragmented RTP packets
    */
    std::vector<RTPPacket> execute(const Frame &frame) override;
};

/**
 * @class EncodingContext
 * @brief Context class sử dụng EncodingStrategy
 *
 * @details
 * EncodingContext quản lý việc chọn strategy phù hợp dựa trên frame size:
 * - Frame ≤ 1400 bytes → SDEncodingStrategy
 * - Frame > 1400 bytes → HDEncodingStrategy
 */
class EncodingContext : public Context<Frame, std::vector<RTPPacket>> {
private:
    std::unique_ptr<EncodingStrategy> sdStrategy_;
    std::unique_ptr<EncodingStrategy> hdStrategy_;
    bool autoDetect_;

    public:
    /**
    * @brief Constructor với auto-detection enabled by default
    */
    EncodingContext();

    /**
    * @brief Enable/disable auto strategy detection
    * @param enable true to enable auto-detection
    */
    void setAutoDetect(bool enable);

    /**
    * @brief Encode frame với appropriate strategy
    * @param frame Frame cần encode
    * @return Vector of RTP packets
    */
    std::vector<RTPPacket> encodeFrame(const Frame &frame);

    /**
    * @brief Get statistics về encoding
    * @param frame Frame cần analyze
    * @return String mô tả encoding strategy sẽ được dùng
    */
    std::string getEncodingInfo(const Frame &frame) const;
};

#endif
