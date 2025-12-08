#ifndef CLIENT_BUFFER_FRAMEBUFFER_HPP
#define CLIENT_BUFFER_FRAMEBUFFER_HPP

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

/**
 * @class FrameBuffer
 * @brief Thread-safe queue for decoded frames (Producer-Consumer pattern)
 * 
 * Producer: FrameReassembler pushes complete frames
 * Consumer: FrameDisplay pops frames for rendering
 */
class FrameBuffer
{
private:
    std::queue<std::vector<uint8_t>> buffer_; ///< Queue of frames (JPEG data)
    size_t maxSize_;

    mutable std::mutex mutex_;
    std::condition_variable cvNotEmpty_; ///< Notify consumer
    std::condition_variable cvNotFull_; ///< Notify producer

    bool closed_;
public:
    /**
    * @brief Constructor
    * @param maxSize Maximum number of frames in buffer (default 100)
    */
    explicit FrameBuffer(size_t maxSize = 100);

    /**
    * @brief Push frame into buffer (blocks if full)
    * @param frame Frame data (JPEG)
    * @return true if pushed, false if buffer closed
    *
    * @details
    * Blocks producer if buffer is full
    * Wakes up consumer when frame available
    */
    bool push(const std::vector<uint8_t>& frameData);

    /**
    * @brief Pop frame from buffer (blocks if empty)
    * @param frame Output frame data (JPEG)
    * @param timeoutMs Timeout in milliseconds (0 = wait forever)
    * @return true if popped, false if timeout or closed
    *
    * @details
    * Blocks producer if buffer is empty
    * Wakes up producer when space available
    */
    bool pop(std::vector<uint8_t>& frameData, int timeoutMs = 0);

    /**
    * @brief Try to pop without blocking
    * @param frameData Output frame data (JPEG)
    * @return true if popped, false if empty
    */
    bool tryPop(std::vector<uint8_t>& frameData);

    /**
    * @brief Try to push without blocking (drops frame if buffer full)
    * @param frameData Frame data to push
    * @return true if pushed, false if buffer full or closed
    */
    bool tryPush(const std::vector<uint8_t>& frameData);

    /**
    * @brief Clear all frame in buffer
    */
    void clear();

    /**
    * @brief Close buffer (no more pushes allowed)
    */
    void close();

    /**
    * @brief Check if buffer is closed
    */
    bool isClosed() const {return closed_;}

    /**
    * @brief Get current buffer size
    */
    size_t size() const {return buffer_.size();}

    /**
    * @brief Get maximum buffer size
    */
    size_t getMaxSize() const {return maxSize_;}

    /**
    * @brief Check if buffer is empty
    */
    bool isEmpty() const {return buffer_.empty();}

    /**
    * @brief Check if buffer is full
    */
    bool isFull() const {return buffer_.size() >= maxSize_;}
};
#endif