#include "video/VideoStream.hpp"
#include "utils/Logger.hpp"

#include <iostream>
#include <vector>

VideoStream::VideoStream(const std::string& filename) : frameNumber_(0), filename_(filename) {
    // Mở file ở chế độ đọc và nhị phân
    std::cout << "[VideoStream] Opening file: " << filename << std::endl;
    videoFile_.open(filename, std::ios::in | std::ios::binary);

    if (!videoFile_.is_open()) {
        throw std::runtime_error("ERROR: Khong the mo file video");
    }

    // Kiểm tra file có rỗng không
    if (videoFile_.peek() == std::ifstream::traits_type::eof()) {
        throw std::runtime_error("ERROR: Video rong");
    }

    totalFrame_ = 0;
}

VideoStream::~VideoStream() {
    if (videoFile_.is_open()) {
        videoFile_.close();
    }
}

std::vector<uint8_t> VideoStream::nextFrame() {
    if (!videoFile_.is_open()) {
        throw std::runtime_error("[VideoStream Error] VideoFile not open!");
        LOG_ERROR("[VideoStream Error] VideoFile not open");
    }

    std::vector<uint8_t> frame;
    bool foundSOI = false;

    int prevByteInt = EOF;
    int currByteInt;q

    while (true) {
        currByteInt = videoFile_.get();
        if (currByteInt == EOF) {
            if (!foundSOI) {
                throw std::runtime_error("ERROR: No more frames (EOF before SOI)");
            } else {
                throw std::runtime_error("ERROR: Truncated frame (EOF before EOI)");
            }
        }

        uint8_t currByte = static_cast<uint8_t>(currByteInt);

        if (!foundSOI) {
            if (prevByteInt == 0xFF && currByte == 0xD8) {
                frame.clear();
                frame.push_back(0xFF);
                frame.push_back(0xD8);
                foundSOI = true;
            }
        } else {
            frame.push_back(currByte);

            if (prevByteInt == 0xFF && currByte == 0xD9) {
                break;
            }
        }

        prevByteInt = currByte;
    }

    if (frame.empty()) {
        throw std::runtime_error("[Video Stream Error] Empty frame read");
        LOG_ERROR("[Video Stream Error] Empty frame read");
    }
    frameNumber_++;
    return frame;
}

void VideoStream::rewind() {
    videoFile_.clear();
    videoFile_.seekg(0, std::ios::beg);
    frameNumber_ = 0;
}

bool VideoStream::hasMoreFrames() const {
    // Kiểm tra file còn tốt và peek() không phải là EOF
    //(videoFile_ là mutable nên ta const_cast hoặc kiểm tra trạng thái)

    // Kiểm tra trạng thái stream và xem thử ký tự tiếp theo
    if (!videoFile_.good())
        return false;

    // Thử nhìn ký tự tiếp theo mà không làm trôi con trỏ
    int c = const_cast<std::ifstream&>(videoFile_).peek();
    return c != EOF;
}
int VideoStream::countFrames() {
    if (!videoFile_.is_open()) {
        std::cerr << "[VideoStream] countFrames: file not open\n";
        return 0;
    }

    videoFile_.clear();
    videoFile_.seekg(0, std::ios::beg);

    int frameCount = 0;
    bool insideFrame = false;

    int prevByteInt = EOF;
    int currByteInt;

    while (true) {
        currByteInt = videoFile_.get();
        if (currByteInt == EOF) {
            break;
        }

        uint8_t currByte = static_cast<uint8_t>(currByteInt);

        if (!insideFrame) {
            if (prevByteInt == 0xFF && currByte == 0xD8) {
                insideFrame = true;
            }
        } else {  // insideFrame == true
            if (prevByteInt == 0xFF && currByte == 0xD9) {
                frameCount++;
                insideFrame = false;
            }
        }

        prevByteInt = currByte;
    }

    // Reset lại cho nextFrame() dùng từ đầu
    videoFile_.clear();
    videoFile_.seekg(0, std::ios::beg);

    std::cout << "[VideoStream] Total frames counted = " << frameCount << "\n";
    return frameCount;
}
