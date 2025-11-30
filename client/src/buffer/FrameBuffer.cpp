#include "buffer/FrameBuffer.hpp"

#include <chrono>

FrameBuffer::FrameBuffer(size_t maxSize) : maxSize_(maxSize), closed_(false) {}

bool FrameBuffer::push(const std::vector<uint8_t>& frameData) {
    std::unique_lock<std::mutex> lock(mutex_);
    // Wait until not full or closed
    cvNotFull_.wait(lock, [this]() { return closed_ || buffer_.size() < maxSize_; });
    if (closed_)
        return false;
    buffer_.push(frameData);
    lock.unlock();
    cvNotEmpty_.notify_one();
    return true;
}

bool FrameBuffer::pop(std::vector<uint8_t>& frameData, int timeoutMs) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (timeoutMs <= 0) {
        cvNotEmpty_.wait(lock, [this]() { return closed_ || !buffer_.empty(); });
        if (buffer_.empty())
            return false;  // closed and empty
    } else {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (buffer_.empty() && !closed_) {
            if (cvNotEmpty_.wait_until(lock, deadline) == std::cv_status::timeout) {
                if (buffer_.empty())
                    return false;  // timeout
            }
        }
        if (buffer_.empty())
            return false;
    }

    frameData = std::move(buffer_.front());
    buffer_.pop();
    lock.unlock();
    cvNotFull_.notify_one();
    return true;
}

bool FrameBuffer::tryPop(std::vector<uint8_t>& frameData) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.empty())
        return false;
    frameData = std::move(buffer_.front());
    buffer_.pop();
    cvNotFull_.notify_one();
    return true;
}

void FrameBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!buffer_.empty())
        buffer_.pop();
    cvNotFull_.notify_all();
}

void FrameBuffer::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    cvNotEmpty_.notify_all();
    cvNotFull_.notify_all();
}
