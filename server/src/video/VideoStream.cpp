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
    // Đọc 5 bytes header chứa kích thước frame
    char header[5];
    videoFile_.read(header, 5);

    // Kiểm tra xem có đủ 5 bytes không
    if (videoFile_.gcount() != 5) {
        throw std::runtime_error("ERROR: Khong doc duoc header frame");
    }

    // Chuyển đổi 5 byte header thành số nguyên (kích thước frame)
    // Phải ép kiểu sang unsigned char để tránh lỗi số âm khi dịch bit
    uint64_t frameLength = 0;
    frameLength |= (static_cast<uint64_t>(static_cast<unsigned char>(header[0])) << 32);
    frameLength |= (static_cast<uint64_t>(static_cast<unsigned char>(header[1])) << 24);
    frameLength |= (static_cast<uint64_t>(static_cast<unsigned char>(header[2])) << 16);
    frameLength |= (static_cast<uint64_t>(static_cast<unsigned char>(header[3])) << 8);
    frameLength |= (static_cast<uint64_t>(static_cast<unsigned char>(header[4])));

    // Nếu frameLength quá lớn (> 50MB), có thể file bị lỗi hoặc parse sai
    if (frameLength > 50 * 1024 * 1024) {
        throw std::runtime_error("Loi: Kich thuoc frame bat thuong (qua lon)");
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