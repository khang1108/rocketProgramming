#include "RTPPacket.hpp"

//ENCODING (ĐÓNG GÓI)
// Nhiệm vụ: Chuyển các biến rời rạc (int, boolean) thành mảng byte thô để gửi qua mạng.
void RTPPacket::encode()
{
    //Byte 0: Version (2 bit), Padding (1 bit), Extension(1 bit), CC(4 bit)
    header_[0] = (version_ << 6) | (padding_ << 5) | (extension_ << 4) | (cc_ & 0x0F);

    //Xây dựng byte thứ hai (Header byte 1)
    header_[1] = (marker_ << 7) | (payloadType_ & 0x7F);

    //Sequence Number (2 byte)
    // Quy tắc mạng (Big-Endian): Byte lớn (đầu) đi trước, Byte nhỏ (đuôi) đi sau.
    // Lấy 8 bit đầu (Cao): Dịch phải 8 bước để đẩy 0x12 xuống cuối
    header_[2] = (sequenceNumber_ >> 8) & 0xFF; 
    
    // Lấy 8 bit cuối (Thấp): Chỉ cần dùng mặt nạ lấy 8 bit cuối
    header_[3] = sequenceNumber_ & 0xFF;

    // BƯỚC 4: Timestamp (32 bit = 4 Bytes)
    // Cắt số 32 bit thành 4 khúc, mỗi khúc 8 bit
    header_[4] = (timestamp_ >> 24) & 0xFF; // Byte cao nhất (MSB)
    header_[5] = (timestamp_ >> 16) & 0xFF; // Byte cao thứ 2
    header_[6] = (timestamp_ >> 8) & 0xFF;  // Byte cao thứ 3
    header_[7] = timestamp_ & 0xFF;         // Byte thấp nhất (LSB)

    // BƯỚC 5: SSRC (32 bit = 4 Bytes)
    // Tương tự Timestamp
    header_[8] = (ssrc_ >> 24) & 0xFF;
    header_[9] = (ssrc_ >> 16) & 0xFF;
    header_[10] = (ssrc_ >> 8) & 0xFF;
    header_[11] = ssrc_ & 0xFF;
}

//DECODING (GIẢI MÃ)
// Nhiệm vụ: Nhận mảng byte thô từ mạng, tách ra thành các biến để sử dụng.
void RTPPacket::decode(const uint8_t *packet, size_t packetSize)
{
    //Kiem tra: Packet phải tồn tại và phải đủ lớn để chứa Header (12 bytes)
    if (packet == nullptr){
        throw std::invalid_argument("ERROR: Khong ton tai packet");
    }
    if (packetSize < HEADER_SIZE){
        throw std::length_error("ERROR: Packet qua nho, khong du chua Header");
    }

    //Tách Byte 0
    version_ = (packet[0] >> 6) & 0x03; 
    padding_ = (packet[0] >> 5) & 0x01;// Lấy bit P
    extension_ = (packet[0] >> 4) & 0x01;// Lấy bit X
    cc_ = packet[0] & 0x0F;// Lấy 4 bit cuối CCCC

    //Tách Byte 1
    marker_ = (packet[1] >> 7) & 0x01; // Lấy bit đầu tiên
    payloadType_ = packet[1] & 0x7F;        // Lấy 7 bit sau

    //Ghép Sequence Number (2 Bytes thành 1 số 16 bit)
    // Byte cao (packet[2]) cần được đẩy lên hàng cao (<< 8)
    // Sau đó cộng (OR) với byte thấp (packet[3])
    sequenceNumber_ = (static_cast<uint16_t>(packet[2]) << 8) | packet[3];

    //Ghép Timestamp (4 Bytes thành 1 số 32 bit)
    timestamp_ = (static_cast<uint32_t>(packet[4]) << 24) |
                 (static_cast<uint32_t>(packet[5]) << 16) |
                 (static_cast<uint32_t>(packet[6]) << 8)  |
                 packet[7];

    //Ghép SSRC (4 Bytes thành 1 số 32 bit)
    ssrc_ = (static_cast<uint32_t>(packet[8]) << 24) |
            (static_cast<uint32_t>(packet[9]) << 16) |
            (static_cast<uint32_t>(packet[10]) << 8) |
            packet[11];

    //Lưu trữ bản sao Header thô
    //Copy 12 byte đầu tiên vào biến header_ để dùng khi cần gửi lại
    std::memcpy(header_.data(), packet, HEADER_SIZE);

    //Tách Payload (Phần dữ liệu video)
    // Payload nằm ngay sau Header (từ byte thứ 12 trở đi)
    size_t payloadSize = packetSize - HEADER_SIZE;
    
    //Cấp phát bộ nhớ cho vector payload_
    payload_.resize(payloadSize);
    
    if (payloadSize > 0) {
        // Copy dữ liệu từ packet vào vector payload_
        // packet + HEADER_SIZE: Con trỏ trỏ đến vị trí bắt đầu của Payload
        std::memcpy(payload_.data(), packet + HEADER_SIZE, payloadSize);
    }
}

size_t RTPPacket::getPacket(uint8_t *buffer, size_t bufferSize) const
{
    size_t totalLen = getLength(); //Tổng kích thước Header + Payload

    //Kiểm tra xem buffer có đủ chỗ chứa hay không
    if (bufferSize < totalLen){
        throw std::length_error("ERROR: Buffer qua nho de chua goi tin");
    }

    //Copy Header vào đầu buffer
    std::memcpy(buffer, header_.data(), HEADER_SIZE);

    //Copy Payload vào ngay sau Header
    if (!payload_.empty()) {
        std::memcpy(buffer + HEADER_SIZE, payload_.data(), payload_.size());
    }

    return totalLen; //Trả về số byte thực tế đã copy
}

// Hàm này dùng để lấy gói tin hoàn chỉnh để gửi qua `sendto()`
// Nó nối [Header] + [Payload] thành một mảng liền mạch.
std::vector<uint8_t> RTPPacket::getPacketVector() const
{
    std::vector<uint8_t> packet;
    //Dành trước bộ nhớ để tăng tốc độ (tránh cấp phát nhiều lần)
    packet.reserve(HEADER_SIZE + payload_.size());

    //Chèn Header (12 bytes) vào đầu
    packet.insert(packet.end(), header_.begin(), header_.end());

    //Chèn Payload (Dữ liệu video) vào sau Header
    packet.insert(packet.end(), payload_.begin(), payload_.end());

    return packet; //Trả về vector chứa toàn bộ gói tin
}

void RTPPacket::setVersion(uint8_t v)
{
    if (v > 3) throw std::invalid_argument("ERROR: Version RTP khong hop le");
    version_ = v;
}

void RTPPacket::setCC(uint8_t cc)
{
    if (cc > 15) throw std::invalid_argument("ERROR: CC chi duoc tu 0-15");
    cc_ = cc;
}

void RTPPacket::setPayloadType(uint8_t pt)
{
    if (pt > 127) throw std::invalid_argument("ERROR: Payload Type chi duoc tu 0-127");
    payloadType_ = pt;
}

void RTPPacket::setPayload(const uint8_t *data, size_t length)
{
    if (data == nullptr && length > 0) {
        throw std::invalid_argument("ERROR: Payload khong co du lieu");
    }
    //Copy dữ liệu từ con trỏ ngoài vào vector nội bộ
    payload_.assign(data, data + length);
}

bool RTPPacket::validate() const
{
    //Kiểm tra cơ bản: Version phải luôn là 2 cho RTP chuẩn
    if (version_ != 2) return false;
    //Kiểm tra xem loại payload có đúng là MJPEG
    if (payloadType_ != MJPEG_TYPE) return false;

    return true;
}

void RTPPacket::printHeader() const
{
    //In ra các thông số header để debug
    std::cout << "[DEBUG RTP] V=" << (int)version_ 
              << " P=" << (int)padding_
              << " M=" << (int)marker_
              << " PT=" << (int)payloadType_
              << " Seq=" << sequenceNumber_
              << " TS=" << timestamp_
              << " Payload=" << payload_.size() << " bytes"
              << std::endl;
}

uint32_t RTPPacket::getCurrentTimestamp()
{
    using namespace std::chrono;
    //Lấy thời điểm hiện tại
    auto now = steady_clock::now();
    //Tính mili-giây từ thời điểm mốc (epoch)
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    //Ép kiểu về số nguyên dương 32 bit
    return static_cast<uint32_t>(ms);
}

int32_t RTPPacket::sequenceDifference(uint16_t seq1, uint16_t seq2)
{
    //Tính khoảng cách giữa 2 gói tin, xử lý trường hợp số sequence bị tràn
    //(VD: Gói 65535 rồi đến gói 0 -> khoảng cách là 1 chứ không phải -65535)
    int32_t diff = seq1 - seq2;
    if (diff > 32768)
        diff -= 65536;
    else if (diff < -32768)
        diff += 65536;
    return diff;
}
