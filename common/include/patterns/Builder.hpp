#ifndef COMMON_PATTERNS_BUILDER_HPP
#define COMMON_PATTERNS_BUILDER_HPP

/**
 * @file Builder.hpp
 * @brief Builder Design Pattern - Interface for constructing complex objects
 *
 * @details
 * Builder pattern separates object construction from its representation:
 * - Fluent interface (method chaining)
 * - Step-by-step object construction
 * - Different representations from same build process
 * - Validation before final object creation
 *
 * Use cases in this project:
 * - RTPPacketBuilder: Construct RTP packets with proper validation
 * - Future: MessageBuilder, ResponseBuilder, etc.
 *
 * @example
 * @code
 * class Product {
 * public:
 *     int id;
 *     std::string name;
 * };
 *
 * class ProductBuilder : public Builder<Product> {
 * private:
 *     int id_ = 0;
 *     std::string name_;
 *
 * public:
 *     ProductBuilder& setId(int id) {
 *         id_ = id;
 *         return *this;
 *     }
 *
 *     ProductBuilder& setName(const std::string& name) {
 *         name_ = name;
 *         return *this;
 *     }
 *
 *     Product build() override {
 *         Product product;
 *         product.id = id_;
 *         product.name = name_;
 *         return product;
 *     }
 *
 *     void reset() override {
 *         id_ = 0;
 *         name_.clear();
 *     }
 * };
 *
 * // Usage
 * Product p = ProductBuilder()
 *     .setId(1)
 *     .setName("Test")
 *     .build();
 * @endcode
 */

/**
 * @class Builder
 * @brief Abstract base class for Builder pattern
 *
 * @tparam T Type of object being built
 */
template <typename T>
class Builder {
  public:
    /**
     * @brief Virtual destructor for polymorphism
     */
    virtual ~Builder() = default;

    /**
     * @brief Build and return the final object
     * @return Constructed object of type T
     * @throws std::runtime_error if required fields not set
     */
    virtual T build() = 0;

    /**
     * @brief Reset builder to initial state
     * @note Useful for reusing builder for multiple objects
     */
    virtual void reset() = 0;
};

#endif  // COMMON_PATTERNS_BUILDER_HPP
