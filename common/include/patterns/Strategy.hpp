#ifndef SERVER_PATTERNS_STRATEGY_HPP
#define SERVER_PATTERNS_STRATEGY_HPP

/**
 * @file Strategy.hpp
 * @brief Strategy Design Pattern - Interface cho các thuật toán có thể thay thế lẫn nhau
 *
 * @details
 * Strategy pattern định nghĩa một họ các thuật toán, đóng gói từng thuật toán,
 * và làm cho chúng có thể hoán đổi cho nhau. Strategy cho phép thuật toán thay đổi
 * độc lập với client sử dụng nó.
 *
 * Trong dự án này, Strategy pattern được sử dụng cho:
 * - **EncodingStrategy**: Chọn giữa SD encoding (single packet) và HD encoding (fragmentation)
 * - Dễ dàng thêm các chiến lược encoding mới (4K, adaptive bitrate, etc.)
 * - Client code không cần biết chi tiết implementation của từng strategy
 *
 * Các concrete strategies:
 * 1. SDEncodingStrategy - Frame < MTU (1400 bytes) → single packet
 * 2. HDEncodingStrategy - Frame > MTU → fragmentation into multiple packets
 *
 * @example
 * @code
 * // Concrete strategy
 * class SDEncodingStrategy : public EncodingStrategy {
 * public:
 *     std::vector<RTPPacket> encode(const Frame& frame) override {
 *         // Simple: one frame = one packet
 *         return { RTPPacket(frame.data, frame.size) };
 *     }
 * };
 *
 * class HDEncodingStrategy : public EncodingStrategy {
 * public:
 *     std::vector<RTPPacket> encode(const Frame& frame) override {
 *         // Complex: fragment frame into multiple packets
 *         std::vector<RTPPacket> packets;
 *         // ... fragmentation logic ...
 *         return packets;
 *     }
 * };
 *
 * // Usage with context
 * EncodingContext context;
 * if (frameSize < 1400) {
 *     context.setStrategy(std::make_unique<SDEncodingStrategy>());
 * } else {
 *     context.setStrategy(std::make_unique<HDEncodingStrategy>());
 * }
 * auto packets = context.encodeFrame(frame);
 * @endcode
 */

/**
 * @class Strategy
 * @brief Abstract base class for Strategy pattern implementation
 *
 * @tparam InputType Type of input data for the strategy
 * @tparam OutputType Type of result produced by the strategy
 */
template <typename InputType, typename OutputType>
class Strategy
{
public:
    /**
     * @brief Virtual destructor cho polymorphism
     */
    virtual ~Strategy() = default;

    /**
     * @brief Thực thi thuật toán strategy
     * @param input Dữ liệu đầu vào cho thuật toán
     * @return Kết quả sau khi xử lý
     * @note Pure virtual function - phải được implement bởi concrete strategy
     */
    virtual OutputType execute(const InputType &input) = 0;
};

/**
 * @class Context
 * @brief Context class chứa tham chiếu đến strategy và delegate công việc cho nó
 * @tparam InputType Kiểu dữ liệu đầu vào
 * @tparam OutputType Kiểu dữ liệu đầu ra
 */
template <typename InputType, typename OutputType>
class Context
{
private:
    std::unique_ptr<Strategy<InputType, OutputType>> strategy_;

public:
    /**
     * @brief Constructor với strategy mặc định
     * @param strategy Strategy ban đầu (optional)
     */
    explicit Context(std::unique_ptr<Strategy<InputType, OutputType>> strategy = nullptr)
        : strategy_(std::move(strategy)) {}

    /**
     * @brief Thiết lập strategy mới
     * @param strategy Strategy mới để sử dụng
     */
    void setStrategy(std::unique_ptr<Strategy<InputType, OutputType>> strategy)
    {
        strategy_ = std::move(strategy);
    }

    /**
     * @brief Thực thi strategy hiện tại
     * @param input Dữ liệu đầu vào
     * @return Kết quả từ strategy
     * @throws std::runtime_error nếu strategy chưa được set
     */
    OutputType executeStrategy(const InputType &input)
    {
        if (!strategy_)
        {
            throw std::runtime_error("Strategy not set");
        }
        return strategy_->execute(input);
    }

    /**
     * @brief Kiểm tra xem strategy đã được set chưa
     * @return true nếu strategy đã được set
     */
    bool hasStrategy() const
    {
        return strategy_ != nullptr;
    }
};

#endif // STRATEGY_HPP
