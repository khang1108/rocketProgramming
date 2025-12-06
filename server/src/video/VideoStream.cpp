#include "video/VideoStream.hpp"

VideoStream::VideoStream(const std::string& filename) : frameNumber_(0), filename_(filename) {
    // Mở file ở chế độ đọc và nhị phân
    videoFile_.open(filename, std::ios::in | std::ios::binary);

    if (!videoFile_.is_open()) {
        throw std::runtime_error("ERROR: Khong the mo file video");
    }

    // Kiểm tra file có rỗng không
    if (videoFile_.peek() == std::ifstream::traits_type::eof()) {
        throw std::runtime_error("ERROR: Video rong");
    }
}

VideoStream::~VideoStream() {
    if (videoFile_.is_open()) {
        videoFile_.close();
    }
}

std::vector<uint8_t> VideoStream::nextFrame() {
    if (!hasMoreFrames()) {
        throw std::runtime_error("ERROR: Khong con frame de doc");
    }
    // Đọc 5 bytes header chứa kích thước frame (ASCII format)
    char header[6];  // 5 bytes + null terminator
    videoFile_.read(header, 5);
    header[5] = '\0';  // Null terminate for string conversion

    // Kiểm tra xem có đủ 5 bytes không
    if (videoFile_.gcount() != 5) {
        throw std::runtime_error("ERROR: Khong doc duoc header frame");
    }

    // Chuyển đổi 5 byte ASCII header thành số nguyên (kích thước frame)
    // Format: "06014" -> 6014 bytes
    int frameLength = 0;
    try {
        frameLength = std::stoi(header);
    } catch (const std::exception& e) {
        throw std::runtime_error("ERROR: Khong the parse frame length tu header: " +
                                 std::string(header));
    }

    // Kiểm tra frameLength hợp lệ
    if (frameLength <= 0 || frameLength > 100000) {  // Max ~100KB per frame for MJPEG
        throw std::runtime_error("ERROR: Frame length khong hop le: " +
                                 std::to_string(frameLength));
    }

    // Đọc dữ liệu ảnh (Payload) dựa trên kích thước vừa tính
    std::vector<uint8_t> frameData(frameLength);
    videoFile_.read(reinterpret_cast<char*>(frameData.data()), frameLength);

    // Kiểm tra xem có đọc đủ dữ liệu không
    if (videoFile_.gcount() != static_cast<std::streamsize>(frameLength)) {
        throw std::runtime_error("Loi: File ket thuc dot ngot khi dang doc frame data");
    }

    // Tăng số thứ tự frame và trả về dữ liệu
    frameNumber_++;
    return frameData;
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