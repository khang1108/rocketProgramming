#ifndef SERVER_PATTERNS_BUILDER_HPP
#define SERVER_PATTERNS_BUILDER_HPP

/**
 * @file Builder.hpp
 * @brief Builder Design Pattern - Interface cho việc xây dựng các đối tượng phức tạp
 *
 * @details
 * Builder pattern tách rời quá trình xây dựng (construction) của một đối tượng phức tạp
 * khỏi cách thức biểu diễn (representation) của nó, cho phép cùng một quá trình xây dựng
 * có thể tạo ra các biểu diễn khác nhau.
 *
 * Trong dự án này, Builder pattern được sử dụng để:
 * - Xây dựng RTP packet với các trường header phức tạp
 * - Tách biệt logic xây dựng packet khỏi class RTPPacket
 * - Dễ dàng thêm/sửa đổi cách xây dựng packet mà không ảnh hưởng code hiện tại
 *
 * @example
 * @code
 * // Concrete builder implementation
 * class RTPPacketBuilder : public Builder<RTPPacket> {
 * public:
 *     RTPPacketBuilder& setVersion(uint8_t v) { version_ = v; return *this; }
 *     RTPPacketBuilder& setPayloadType(uint8_t pt) { payloadType_ = pt; return *this; }
 *     RTPPacket build() override { return RTPPacket(...); }
 * };
 *
 * // Usage
 * RTPPacket packet = RTPPacketBuilder()
 *     .setVersion(2)
 *     .setPayloadType(26)
 *     .setSequenceNumber(100)
 *     .setTimestamp(123456)
 *     .setPayload(data, size)
 *     .build();
 * @endcode
 */

/**
 * @class Builder
 * @brief Abstract base class for Builder pattern implementation
 *
 * @tparam T Type of object to build
 */
template <typename T>
class Builder
{
public:
    /**
     * @brief Virtual destructor cho polymorphism
     */
    virtual ~Builder() = default;

    /**
     * @brief Xây dựng và trả về đối tượng cuối cùng
     * @return Đối tượng đã được xây dựng hoàn chỉnh
     * @note Pure virtual function - phải được implement bởi concrete builder
     */
    virtual T build() = 0;

    /**
     * @brief Reset builder về trạng thái ban đầu
     * @note Optional - có thể override nếu cần reuse builder
     */
    virtual void reset() {}
};

#endif // BUILDER_HPP
