#ifndef ENCODINGSTRATEGY_HPP
#define ENCODINGSTRATEGY_HPP

#include "RTPPacket.hpp"
#include "RTPPacketBuilder.hpp"
#include "../patterns/Strategy.hpp"
#include <vector>
#include <memory>
#include <algorithm>

/**
 * @struct Frame
 * @brief Đại diện cho một video frame cần encode
 */
struct Frame
{
    std::vector<uint8_t> data; ///< Frame data (JPEG bytes)
    uint16_t sequenceNumber;   ///< Starting sequence number for this frame
    uint32_t timestamp;        ///< Frame timestamp
    uint32_t ssrc;             ///< Server SSRC

    Frame(const std::vector<uint8_t> &d, uint16_t seq, uint32_t ts, uint32_t src)
        : data(d), sequenceNumber(seq), timestamp(ts), ssrc(src) {}
};

/**
 * @class EncodingStrategy
 * @brief Strategy Pattern base class cho các thuật toán encoding RTP packets
 *
 * @details
 * EncodingStrategy định nghĩa interface cho các chiến lược encoding khác nhau:
 * - **SDEncodingStrategy**: Cho SD video (frame < MTU) - một frame = một packet
 * - **HDEncodingStrategy**: Cho HD video (frame > MTU) - fragmentation cần thiết
 *
 * Strategy pattern cho phép:
 * 1. Chọn thuật toán encoding runtime dựa trên frame size
 * 2. Dễ dàng thêm strategies mới (4K, adaptive bitrate, etc.)
 * 3. Client code không cần biết chi tiết implementation
 * 4. Test từng strategy độc lập
 *
 * @see Strategy - Base strategy pattern interface
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
 *
 * Đặc điểm:
 * - No fragmentation needed
 * - Marker bit luôn = 1 (mỗi packet là frame hoàn chỉnh)
 * - Sequence number tăng 1 cho mỗi frame
 * - Low latency, đơn giản
 *
 * @example
 * @code
 * SDEncodingStrategy strategy;
 * Frame frame(jpegData, seqNum, timestamp, ssrc);
 * std::vector<RTPPacket> packets = strategy.execute(frame);
 * // packets.size() == 1
 * @endcode
 */
class SDEncodingStrategy : public EncodingStrategy
{
public:
    /**
     * @brief Encode một frame thành một RTP packet
     * @param frame Frame cần encode
     * @return Vector chứa 1 RTP packet
     */
    std::vector<RTPPacket> execute(const Frame &frame) override
    {
        std::vector<RTPPacket> packets;

        // Build single packet
        RTPPacket packet = RTPPacketBuilder()
                               .setPayloadType(RTPPacket::MJPEG_TYPE)
                               .setSequenceNumber(frame.sequenceNumber)
                               .setTimestamp(frame.timestamp)
                               .setSSRC(frame.ssrc)
                               .setMarker(1) // Single packet = complete frame, marker = 1
                               .setPayload(frame.data)
                               .build();

        packets.push_back(std::move(packet));
        return packets;
    }
};

/**
 * @class HDEncodingStrategy
 * @brief Strategy cho HD video - fragmentation into multiple packets
 *
 * @details
 * Được sử dụng khi frame size > MAX_PAYLOAD_SIZE (1400 bytes).
 * Fragment frame thành nhiều packets, mỗi packet ≤ MTU.
 *
 * Đặc điểm:
 * - Frame được chia thành chunks ≤ 1400 bytes
 * - Mỗi chunk tạo thành 1 RTP packet
 * - Marker bit = 0 cho tất cả packets trừ packet cuối (marker = 1)
 * - Tất cả fragments có SAME timestamp
 * - Sequence number tăng cho mỗi packet
 *
 * Fragmentation algorithm:
 * 1. Tính số packets cần: numPackets = ceil(frameSize / MAX_PAYLOAD_SIZE)
 * 2. For each chunk:
 *    - Extract chunk data (≤ 1400 bytes)
 *    - Set marker = 1 nếu là chunk cuối, 0 otherwise
 *    - Build RTP packet với sequence number tăng dần
 *    - Same timestamp cho tất cả packets
 *
 * @example
 * @code
 * HDEncodingStrategy strategy;
 * Frame largeFrame(hdJpegData, seqNum, timestamp, ssrc);  // 50KB frame
 * std::vector<RTPPacket> packets = strategy.execute(largeFrame);
 * @endcode
 */
class HDEncodingStrategy : public EncodingStrategy
{
public:
    /**
     * @brief Encode một large frame thành multiple RTP packets
     * @param frame Frame cần encode (frame.data.size() > MAX_PAYLOAD_SIZE)
     * @return Vector chứa fragmented RTP packets
     *
     * @note Tất cả packets có same timestamp
     * @note Sequence number increment cho mỗi packet
     * @note Marker bit = 1 chỉ cho packet cuối
     */
    std::vector<RTPPacket> execute(const Frame &frame) override
    {
        std::vector<RTPPacket> packets;

        const size_t frameSize = frame.data.size();
        const size_t maxPayload = RTPPacket::MAX_PAYLOAD_SIZE;
        size_t offset = 0;
        uint16_t currentSeq = frame.sequenceNumber;

        // Fragment frame into chunks
        while (offset < frameSize)
        {
            // Calculate chunk size
            size_t chunkSize = std::min(maxPayload, frameSize - offset);
            bool isLast = (offset + chunkSize >= frameSize);

            // Build packet for this fragment
            RTPPacket packet = RTPPacketBuilder()
                                .setPayloadType(RTPPacket::MJPEG_TYPE)
                                .setSequenceNumber(currentSeq++)
                                .setTimestamp(frame.timestamp) // SAME timestamp for all fragments
                                .setSSRC(frame.ssrc)
                                .setMarker(isLast ? 1 : 0) // Marker = 1 only for last fragment
                                .setPayload(&frame.data[offset], chunkSize)
                                .build();

            packets.push_back(std::move(packet));
            offset += chunkSize;
        }

        return packets;
    }
};

/**
 * @class EncodingContext
 * @brief Context class sử dụng EncodingStrategy
 *
 * @details
 * EncodingContext quản lý việc chọn strategy phù hợp dựa trên frame size:
 * - Frame ≤ 1400 bytes → SDEncodingStrategy
 * - Frame > 1400 bytes → HDEncodingStrategy
 *
 * Context có thể:
 * - Auto-detect strategy dựa trên frame size (recommend)
 * - Manually set strategy (cho testing hoặc special cases)
 *
 * @example Sử dụng auto-detection (recommended):
 * @code
 * EncodingContext context;
 *
 * Frame sdFrame(smallData, seq, ts, ssrc);  // size = 800 bytes
 * auto packets = context.encodeFrame(sdFrame);  // Auto chọn SDStrategy
 *
 * Frame hdFrame(largeData, seq, ts, ssrc);  // size = 50KB
 * auto packets = context.encodeFrame(hdFrame);  // Auto chọn HDStrategy
 * @endcode
 *
 * @example Manual strategy selection:
 * @code
 * EncodingContext context;
 * context.setStrategy(std::make_unique<HDEncodingStrategy>());
 * auto packets = context.encodeFrame(frame);  // Force HD strategy
 * @endcode
 */
class EncodingContext : public Context<Frame, std::vector<RTPPacket>>
{
private:
    std::unique_ptr<EncodingStrategy> sdStrategy_;
    std::unique_ptr<EncodingStrategy> hdStrategy_;
    bool autoDetect_;

public:
    /**
     * @brief Constructor với auto-detection enabled by default
     */
    EncodingContext()
        : Context<Frame, std::vector<RTPPacket>>(),
        sdStrategy_(std::make_unique<SDEncodingStrategy>()),
        hdStrategy_(std::make_unique<HDEncodingStrategy>()),
        autoDetect_(true) {}

    /**
     * @brief Enable/disable auto strategy detection
     * @param enable true to enable auto-detection
     */
    void setAutoDetect(bool enable)
    {
        autoDetect_ = enable;
    }

    /**
     * @brief Encode frame với appropriate strategy
     * @param frame Frame cần encode
     * @return Vector of RTP packets
     *
     * @details
     * Nếu auto-detect enabled:
     * - Chọn SDStrategy nếu frame size ≤ MAX_PAYLOAD_SIZE
     * - Chọn HDStrategy nếu frame size > MAX_PAYLOAD_SIZE
     *
     * Nếu auto-detect disabled:
     * - Sử dụng strategy đã set manually
     */
    std::vector<RTPPacket> encodeFrame(const Frame &frame)
    {
        if (autoDetect_)
        {
            // Auto-select strategy based on frame size
            if (frame.data.size() <= RTPPacket::MAX_PAYLOAD_SIZE)
            {
                return sdStrategy_->execute(frame);
            }
            else
            {
                return hdStrategy_->execute(frame);
            }
        }
        else
        {
            // Use manually set strategy
            if (!hasStrategy())
            {
                throw std::runtime_error("Strategy not set and auto-detect disabled");
            }
            return executeStrategy(frame);
        }
    }

    /**
     * @brief Get statistics về encoding
     * @param frame Frame cần analyze
     * @return String mô tả encoding strategy sẽ được dùng
     */
    std::string getEncodingInfo(const Frame &frame) const
    {
        size_t frameSize = frame.data.size();
        if (frameSize <= RTPPacket::MAX_PAYLOAD_SIZE)
        {
            return "SD Encoding (single packet): " + std::to_string(frameSize) + " bytes";
        }
        else
        {
            size_t numPackets = (frameSize + RTPPacket::MAX_PAYLOAD_SIZE - 1) / RTPPacket::MAX_PAYLOAD_SIZE;
            return "HD Encoding (" + std::to_string(numPackets) + " packets): " + std::to_string(frameSize) + " bytes";
        }
    }
};

#endif // ENCODINGSTRATEGY_HPP
