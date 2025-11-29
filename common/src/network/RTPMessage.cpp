#include "RTPMessage.hpp"

// Hàm cắt chuỗi dựa trên ký tự phân cách (delimiter)
static std::vector<std::string> split(const std::string &s, char delimiter) 
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        // Giao thức mạng dùng \r\n, nên khi cắt theo \n sẽ dư lại \r
        // Ta cần xóa ký tự \r ở cuối nếu có
        if (!token.empty() && token.back() == '\r') {
            token.pop_back();
        }
        if (!token.empty()) { // Bỏ qua dòng trống
            tokens.push_back(token);
        }
    }
    return tokens;
}

RTPMessage::Request RTPMessage::parseRequest(const std::string& message) 
{
    Request req;
    
    //Tách thông điệp thành từng dòng
    std::vector<std::string> lines = split(message, '\n');
    if (lines.empty()) {
        throw std::runtime_error("ERROR: Thong diep RTPS rong");
    }

    //Phân tích dòng đầu tiên (Request Line)
    //Cấu trúc: METHOD URL VERSION (Ví dụ: SETUP movie.Mjpeg RTSP/1.0)
    std::istringstream firstLineStream(lines[0]);
    std::string version;
    firstLineStream >> req.method >> req.url >> version;

    if (req.method.empty() || req.url.empty()) {
        throw std::runtime_error("ERROR: Yeu cau request khong hop le");
    }

    //Phân tích các dòng Header phía dưới
    for (size_t i = 1; i < lines.size(); ++i) {
        std::string line = lines[i];
        size_t colonPos = line.find(':'); // Tìm dấu hai chấm ngăn cách Key: Value
        
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            //Xóa khoảng trắng thừa ở đầu giá trị (nếu có)
            if (!value.empty() && value[0] == ' ') {
                value.erase(0, 1);
            }

            //Lưu vào map headers
            req.headers[key] = value;

            // Nếu gặp CSeq, lưu riêng ra biến int để dễ dùng sau này
            if (key == "CSeq") {
                try {
                    req.cseq = std::stoi(value);
                } catch (...) {
                    req.cseq = 0; // Mặc định nếu lỗi
                }
            }
        }
    }
    return req;
}

RTPMessage::Response RTPMessage::parseResponse(const std::string& message) 
{
    Response res;
    std::vector<std::string> lines = split(message, '\n');

    if (lines.empty()) {
        throw std::runtime_error("ERROR: Thong diep RTPS rong");
    }

    //Phân tích dòng đầu tiên (Status Line)
    //Cấu trúc: VERSION CODE REASON (Ví dụ: RTSP/1.0 200 OK)
    std::istringstream firstLineStream(lines[0]);
    std::string version;
    firstLineStream >> version >> res.statusCode;
    
    //Phần Reason (lý do) có thể chứa khoảng trắng (VD: Not Found)
    //nên ta lấy toàn bộ phần còn lại của dòng
    std::getline(firstLineStream, res.reason);
    
    //Xóa khoảng trắng thừa đầu dòng do getline để lại
    if (!res.reason.empty() && res.reason[0] == ' ') {
        res.reason.erase(0, 1);
    }

    // 2. Phân tích các dòng Header
    for (size_t i = 1; i < lines.size(); ++i) {
        std::string line = lines[i];
        size_t colonPos = line.find(':');
        
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            if (!value.empty() && value[0] == ' ') value.erase(0, 1);

            res.headers[key] = value;

            if (key == "CSeq") {
                try {
                    res.cseq = std::stoi(value);
                } catch (...) {
                    res.cseq = 0;
                }
            }
        }
    }
    return res;
}

std::string RTPMessage::buildRequest(const std::string& method, const std::string url, int cseq,
                                     const std::map<std::string, std::string>& headers)
{
    std::stringstream ss;
    
    //Dòng 1: METHOD URL RTSP/1.0
    ss << method << " " << url << " RTSP/1.0\r\n";
    
    //Header bắt buộc: CSeq
    ss << "CSeq: " << cseq << "\r\n";
    
    //Các header khác
    for (const auto& pair : headers) {
        ss << pair.first << ": " << pair.second << "\r\n";
    }
    
    //Kết thúc Header bằng dòng trống
    ss << "\r\n";
    
    return ss.str();
}

std::string RTPMessage::buildResponse(int statusCode, std::string reason, int cseq,
                                      const std::map<std::string, std::string>& headers) 
{
    std::stringstream ss;
    
    //Dòng 1: RTSP/1.0 CODE REASON
    ss << "RTSP/1.0 " << statusCode << " " << reason << "\r\n";
    
    //Header bắt buộc: CSeq
    ss << "CSeq: " << cseq << "\r\n";
    
    //Các header khác
    for (const auto& pair : headers) {
        ss << pair.first << ": " << pair.second << "\r\n";
    }
    
    //Kết thúc Header bằng dòng trống
    ss << "\r\n";
    
    return ss.str();
}

std::string RTPMessage::getHeader(const std::map<std::string, std::string>& headers,
                                  const std::string& key, const std::string& defaultValue) 
{
    //Tìm kiếm trong map headers
    auto it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return defaultValue; //Trả về mặc định nếu không tìm thấy
}

std::string RTPMessage::statusCodeToReason(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 454: return "Session Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}
