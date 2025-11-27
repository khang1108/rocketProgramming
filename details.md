# PHÂN TÍCH CHI TIẾT DỰ ÁN ROCKET PROGRAMMING
## Ứng Dụng Streaming Video Thời Gian Thực Sử Dụng RTP/RTSP

---

## 📋 MỤC LỤC
1. [Tại Sao Dự Án Này Được Xây Dựng?](#tại-sao-dự-án-này-được-xây-dựng)
2. [Kiến Trúc Tổng Quan](#kiến-trúc-tổng-quan)
3. [Phân Tích Chi Tiết Từng File](#phân-tích-chi-tiết-từng-file)
4. [Các Thuật Toán Sử Dụng](#các-thuật-toán-sử-dụng)
5. [Luồng Hoạt Động Của Hệ Thống](#luồng-hoạt-động-của-hệ-thống)

---

## 🎯 TẠI SAO DỰ ÁN NÀY ĐƯỢC XÂY DỰNG?

### 1. Mục Đích Học Tập và Nghiên Cứu
Dự án này được xây dựng để:
- **Học tập về mạng máy tính**: Hiểu cách thức hoạt động của các giao thức streaming video như RTSP và RTP
- **Thực hành lập trình mạng**: Áp dụng kiến thức về socket programming, TCP/UDP, multi-threading
- **Nghiên cứu streaming video**: Tìm hiểu cách video được truyền tải qua mạng trong thời gian thực

### 2. Ứng Dụng Thực Tế
- **Video streaming**: Tương tự như YouTube, Netflix nhưng ở mức độ đơn giản hơn
- **Hệ thống giám sát**: Có thể mở rộng để xây dựng hệ thống camera giám sát
- **Video conference**: Nền tảng cơ bản cho các ứng dụng hội nghị trực tuyến

### 3. Tại Sao Sử Dụng RTP/RTSP?
- **RTSP (Real-Time Streaming Protocol)**: 
  - Dùng để điều khiển luồng video (play, pause, stop)
  - Chạy trên TCP để đảm bảo độ tin cậy
  - Tương tự HTTP nhưng có trạng thái (stateful)
  
- **RTP (Real-Time Transport Protocol)**:
  - Dùng để truyền dữ liệu video thực tế
  - Chạy trên UDP để đảm bảo tốc độ (không cần xác nhận từng gói)
  - Hỗ trợ timestamp và sequence number để đồng bộ

### 4. Kiến Trúc Client-Server
- **Server**: Phục vụ nhiều client đồng thời, mỗi client một luồng riêng
- **Client**: Kết nối đến server, yêu cầu video và hiển thị

---

## 🏗️ KIẾN TRÚC TỔNG QUAN

```
┌─────────────┐                    ┌─────────────┐
│   CLIENT    │                    │   SERVER    │
│             │                    │             │
│  ┌────────┐ │                    │  ┌────────┐ │
│  │  GUI   │ │                    │  │Server │ │
│  │Window │ │                    │  │Window │ │
│  └───┬────┘ │                    │  └───┬────┘ │
│      │      │                    │      │      │
│  ┌───▼────┐ │                    │  ┌───▼────┐ │
│  │RTSP    │ │◄───RTSP (TCP)─────►│  │Client │ │
│  │Client  │ │                    │  │Session│ │
│  └───┬────┘ │                    │  └───┬────┘ │
│      │      │                    │      │      │
│  ┌───▼────┐ │                    │  ┌───▼────┐ │
│  │RTP     │ │◄───RTP (UDP)──────►│  │Video   │ │
│  │Receiver│ │                    │  │Stream  │ │
│  └────────┘ │                    │  └────────┘ │
└─────────────┘                    └─────────────┘
```

### Các Thành Phần Chính:
1. **Server**: Nhận kết nối từ client, xử lý RTSP requests, gửi RTP packets
2. **Client**: Gửi RTSP commands, nhận và hiển thị video
3. **Common Library**: Chứa class RtpPacket để encode/decode RTP packets

---

## 📁 PHÂN TÍCH CHI TIẾT TỪNG FILE

### 🔧 1. FILE CẤU HÌNH BUILD

#### **CMakeLists.txt (Root)**
**Mục đích**: File cấu hình chính cho CMake build system

**Logic bên trong**:
```cmake
cmake_minimum_required(VERSION 3.16)  # Yêu cầu CMake phiên bản tối thiểu
project(Socket LANGUAGES CXX)         # Đặt tên project và ngôn ngữ C++

set(CMAKE_CXX_STANDARD 17)            # Sử dụng C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # Bắt buộc phải có C++17

# Cấu hình Qt6
set(CMAKE_AUTOMOC ON)                 # Tự động xử lý MOC (Meta-Object Compiler)
set(CMAKE_AUTORCC ON)                 # Tự động xử lý resource files
set(CMAKE_AUTOUIC ON)                 # Tự động xử lý UI files

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)
# Tìm và liên kết Qt6 với các module: Widgets (GUI), Core, Network (socket)

add_subdirectory(src/common)          # Build thư viện common trước
add_subdirectory(src/client)          # Build client app
add_subdirectory(src/server)          # Build server app
```

**Tại sao cần file này?**
- CMake là công cụ cross-platform để build project
- Tự động tìm dependencies (Qt6)
- Quản lý thứ tự build (common → client/server)

---

#### **src/common/CMakeLists.txt**
**Mục đích**: Cấu hình build cho thư viện chung

**Logic**:
```cmake
add_library(common_lib STATIC src/RtpPacket.h)
# Tạo thư viện tĩnh (static library) từ RtpPacket.h
# Static library: code được compile vào executable, không cần .dll/.so

target_include_directories(common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Thêm thư mục include vào đường dẫn tìm kiếm header files
# PUBLIC: cả common_lib và các target liên kết với nó đều có thể dùng
```

**Tại sao dùng static library?**
- RtpPacket được dùng chung bởi cả client và server
- Tránh duplicate code
- Dễ quản lý và maintain

---

#### **src/server/CMakeLists.txt**
**Mục đích**: Cấu hình build cho server application

**Logic**:
```cmake
add_executable(server_app
    src/main.cpp           # Entry point
    src/Server.cpp         # Class Server implementation
    src/ClientSession.cpp   # Class ClientSession implementation
    src/VideoStream.cpp    # Class VideoStream (đọc video file)
    src/ServerWindow.cpp   # GUI window cho server
)

target_include_directories(server_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # Thêm thư mục include để tìm header files
)

target_link_libraries(server_app PRIVATE
    Qt6::Widgets      # GUI components
    Qt6::Network      # Network sockets
    Qt6::Core         # Core Qt functionality
    common_lib        # Thư viện RtpPacket
)

if(WIN32)
    target_link_libraries(server_app PRIVATE ws2_32)
    # Trên Windows, cần link với WinSock2 library
endif()
```

**Giải thích**:
- `add_executable`: Tạo file thực thi từ các source files
- `target_link_libraries`: Liên kết các thư viện cần thiết
- `PRIVATE`: Các thư viện này chỉ dùng trong server_app, không truyền cho target khác

---

#### **src/client/CMakeLists.txt**
**Mục đích**: Cấu hình build cho client application

**Logic**: Tương tự server nhưng chỉ có:
- `ClientWindow.cpp`: GUI cho client
- `RTSPClient.cpp`: Xử lý RTSP protocol
- `main.cpp`: Entry point

---

### 📚 2. THƯ VIỆN CHUNG (Common Library)

#### **src/common/src/RtpPacket.h** và **src/common/include/RtpPacket.cpp**
**Trạng thái**: Hiện tại đang trống (placeholder)

**Mục đích dự kiến**: Class để encode/decode RTP packets

**Logic cần implement**:

```cpp
class RtpPacket {
private:
    static const int HEADER_SIZE = 12;  // RTP header luôn 12 bytes
    
    // Các trường trong RTP header
    int version;        // Version (phải là 2)
    int padding;        // Có padding không
    int extension;      // Có extension header không
    int cc;             // Contributing source count
    int marker;         // Marker bit
    int payloadType;    // Loại payload (26 = MJPEG)
    int sequenceNumber; // Số thứ tự packet
    int timestamp;      // Thời gian
    int ssrc;           // Synchronization source identifier
    
    byte header[HEADER_SIZE];  // Header dạng byte array
    byte* payload;              // Dữ liệu video frame
    int payloadSize;            // Kích thước payload
    
public:
    // Constructor: Tạo RTP packet từ các tham số
    RtpPacket(int payloadType, int seqNum, int timestamp, 
              byte* data, int dataLength);
    
    // Constructor: Parse RTP packet từ byte stream
    RtpPacket(byte* packet, int packetSize);
    
    // Encode: Đóng gói header và payload thành RTP packet
    void encode(...);
    
    // Decode: Tách header và payload từ byte stream
    void decode(byte* packet);
    
    // Getter methods
    int getSequenceNumber();
    int getTimestamp();
    byte* getPayload();
    byte* getPacket();  // Trả về toàn bộ packet (header + payload)
};
```

**Cấu trúc RTP Header (12 bytes)**:
```
Byte 0: [V=2][P][X][CC=0]          (Version, Padding, Extension, CC)
Byte 1: [M][PT=26]                 (Marker, Payload Type = MJPEG)
Byte 2-3: Sequence Number          (16-bit, big-endian)
Byte 4-7: Timestamp                (32-bit, big-endian)
Byte 8-11: SSRC                    (32-bit, Synchronization Source)
```

**Tại sao cần RTP packetization?**
- Video frame rất lớn, cần chia nhỏ thành packets
- RTP header chứa thông tin để client:
  - Sắp xếp lại thứ tự packets (sequence number)
  - Đồng bộ thời gian (timestamp)
  - Xác định loại dữ liệu (payload type)

---

### 🖥️ 3. SERVER IMPLEMENTATION

#### **Server/Server/Server.h**
**Mục đích**: Định nghĩa class Server - quản lý listening socket

**Logic chi tiết**:

```cpp
// Xử lý đa nền tảng (Windows/Linux)
#ifdef _WIN32
    #include<WinSock2.h>      // Windows Socket API
    #include<WS2tcpip.h>       // TCP/IP utilities
    #pragma comment(lib, "ws2_32.lib")  // Tự động link library
    using socklen_t = int;
#else
    #include<sys/socket.h>     // POSIX sockets
    #include<netinet/in.h>     // Internet address structures
    #include<arpa/inet.h>      // IP address conversion
    #include<unistd.h>         // close() function
    using SOCKET = int;        // Trên Linux, socket là int
    const int INVALID_SOCKET = -1;
    const int SOCKET_ERROR = -1;
    #define closesocket close  // Macro để dùng close() thay vì closesocket()
#endif

class Server {
private:
    SOCKET listenSocket;  // Socket để lắng nghe kết nối mới
    int port;             // Port server lắng nghe (mặc định 5555)
    
public:
    Server(int port);     // Constructor: khởi tạo với port
    ~Server();            // Destructor: đóng socket
    void run();           // Hàm chính: bắt đầu server
};
```

**Tại sao cần cross-platform code?**
- Windows dùng WinSock2 API (khác với POSIX)
- Linux/Unix dùng POSIX socket API
- Code này cho phép compile trên cả 2 platform

**Các thành phần**:
- `listenSocket`: Socket chờ kết nối từ client
- `port`: Cổng mạng (5555) - giống như địa chỉ nhà

---

#### **Server/Server/Server.cpp**
**Mục đích**: Implementation của Server class

##### **Class SocketHelper (RAII Pattern)**
```cpp
class SocketHelper {
public:
    SocketHelper() {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    #endif
    }
    ~SocketHelper() {
    #ifdef _WIN32
        WSACleanup();
    #endif
    }
};
```

**Logic**:
- **RAII (Resource Acquisition Is Initialization)**: 
  - Constructor: Khởi tạo WinSock khi object được tạo
  - Destructor: Dọn dẹp WinSock khi object bị hủy
- **WSAStartup**: Trên Windows, phải gọi hàm này trước khi dùng socket
- **WSACleanup**: Dọn dẹp khi không dùng nữa
- **Tại sao dùng RAII?**: Đảm bảo resource luôn được giải phóng, kể cả khi có exception

---

##### **Server::Server(int port) - Constructor**
```cpp
Server::Server(int port) : port(port), listenSocket(INVALID_SOCKET) {};
```

**Logic**:
- **Initialization list**: Khởi tạo `port` và `listenSocket` ngay khi object được tạo
- `INVALID_SOCKET`: Giá trị đặc biệt cho biết socket chưa được tạo
- **Tại sao dùng initialization list?**: Nhanh hơn và an toàn hơn so với gán trong constructor body

---

##### **Server::~Server() - Destructor**
```cpp
Server::~Server() {
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }
}
```

**Logic**:
- Kiểm tra socket có hợp lệ không trước khi đóng
- `closesocket()`: Đóng socket và giải phóng tài nguyên
- **Tại sao cần đóng socket?**: Tránh resource leak, giải phóng port để dùng lại

---

##### **Server::run() - Hàm Chính**
Đây là hàm quan trọng nhất, chứa toàn bộ logic của server.

**Bước 1: Tạo Socket**
```cpp
listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
```

**Giải thích chi tiết**:
- `socket()`: Tạo socket descriptor
- `AF_INET`: Address Family - IPv4 (có thể dùng AF_INET6 cho IPv6)
- `SOCK_STREAM`: Loại socket - TCP (reliable, connection-oriented)
  - `SOCK_DGRAM` = UDP (unreliable, connectionless) - dùng cho RTP sau này
- `IPPROTO_TCP`: Giao thức TCP
- **Kết quả**: Trả về socket descriptor (số nguyên) hoặc INVALID_SOCKET nếu lỗi

**Tại sao dùng TCP cho RTSP?**
- RTSP cần đảm bảo các lệnh điều khiển (SETUP, PLAY, PAUSE) được truyền đúng
- TCP có cơ chế retransmission nếu packet bị mất
- UDP nhanh hơn nhưng không đảm bảo, phù hợp cho RTP (video data)

---

**Bước 2: Định Nghĩa Địa Chỉ Server**
```cpp
sockaddr_in serverAddr;
serverAddr.sin_family = AF_INET;              // IPv4
serverAddr.sin_port = htons(this->port);       // Port (chuyển sang network byte order)
serverAddr.sin_addr.s_addr = INADDR_ANY;      // Lắng nghe trên tất cả interfaces
```

**Giải thích**:
- `sockaddr_in`: Structure chứa địa chỉ IPv4
- `sin_family`: Loại địa chỉ (AF_INET = IPv4)
- `htons()`: Host TO Network Short - chuyển port từ host byte order sang network byte order
  - **Tại sao cần?**: Máy tính khác nhau có thể dùng byte order khác nhau (big-endian vs little-endian)
  - Network byte order luôn là big-endian
- `INADDR_ANY`: Lắng nghe trên tất cả network interfaces (0.0.0.0)
  - Nếu chỉ muốn lắng nghe trên 1 interface cụ thể, dùng `inet_addr("192.168.1.100")`

**Ví dụ**:
```cpp
port = 5555
htons(5555) = 0x15B3 (trong network byte order)
```

---

**Bước 3: Bind Socket Vào Port**
```cpp
if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    cerr << "[Server] ERROR: bind() that bai!!!\n";
    closesocket(listenSocket);
    return;
}
```

**Logic**:
- `bind()`: Gắn socket vào địa chỉ IP và port cụ thể
- `(sockaddr*)&serverAddr`: Ép kiểu vì bind() nhận `sockaddr*` (generic)
- `sizeof(serverAddr)`: Kích thước của structure
- **Lỗi có thể xảy ra**:
  - Port đã được sử dụng bởi process khác
  - Không có quyền bind vào port < 1024 (privileged ports)
  - Firewall chặn

**Tại sao cần bind?**
- Socket phải biết lắng nghe ở đâu (IP + port)
- Không bind thì socket không có địa chỉ, client không biết kết nối đến đâu

---

**Bước 4: Lắng Nghe Kết Nối**
```cpp
if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
    cerr << "[Server] ERROR: listen() that bai!!!\n";
    closesocket(listenSocket);
    return;
}
```

**Logic**:
- `listen()`: Chuyển socket sang chế độ passive - sẵn sàng nhận kết nối
- `SOMAXCONN`: Số lượng kết nối tối đa trong queue chờ xử lý
  - Trên Windows thường là 200
  - Trên Linux có thể là 128 hoặc lớn hơn
- **Sau listen()**: Socket không thể dùng để gửi/nhận data trực tiếp, chỉ dùng để accept()

**Queue của listen()**:
```
Client 1 ──┐
Client 2 ──┤
Client 3 ──┼──► [Queue] ──► accept() ──► Xử lý
Client 4 ──┤
Client 5 ──┘
```

---

**Bước 5: Vòng Lặp Accept Kết Nối**
```cpp
while (true) {
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    
    SOCKET clientSocket = accept(listenSocket, 
                                  (sockaddr*)&clientAddr, 
                                  &clientAddrSize);
    
    if (clientSocket == INVALID_SOCKET) {
        cerr << "[Server] ERROR: accept() that bai!!!\n";
        continue;  // Bỏ qua lỗi, tiếp tục chờ client khác
    }
    
    cout << "[Server] Da co ket noi. Tao luong ClientSession ...\n";
    
    ClientSession* session = new ClientSession(clientSocket);
    
    thread clientThread(&ClientSession::run, session);
    clientThread.detach();
}
```

**Logic chi tiết**:

1. **accept() - Chấp Nhận Kết Nối**:
   - **Blocking call**: Hàm này sẽ chờ cho đến khi có client kết nối
   - Trả về socket mới (`clientSocket`) để giao tiếp với client cụ thể
   - `clientAddr`: Chứa địa chỉ IP và port của client
   - `clientAddrSize`: Kích thước của clientAddr (input/output parameter)

2. **Multi-threading**:
   ```cpp
   ClientSession* session = new ClientSession(clientSocket);
   thread clientThread(&ClientSession::run, session);
   clientThread.detach();
   ```
   - **Tại sao cần thread?**: 
     - Mỗi client cần xử lý độc lập
     - Nếu không dùng thread, server chỉ phục vụ được 1 client tại một thời điểm
     - Với thread, server có thể phục vụ nhiều client đồng thời
   
   - **detach()**: 
     - Thread chạy độc lập, không cần join()
     - Khi thread kết thúc, tự động cleanup
     - **Lưu ý**: Memory của `session` phải được quản lý cẩn thận (hiện tại dùng `delete this` trong ClientSession)

3. **Vòng lặp vô hạn**:
   - Server chạy liên tục, luôn sẵn sàng nhận client mới
   - Khi 1 client disconnect, server vẫn tiếp tục chờ client khác

**Mô hình hoạt động**:
```
Main Thread (Server::run):
  └─► accept() ──► Client 1 ──► Thread 1 (ClientSession::run)
  └─► accept() ──► Client 2 ──► Thread 2 (ClientSession::run)
  └─► accept() ──► Client 3 ──► Thread 3 (ClientSession::run)
  └─► ...
```

---

##### **main() - Entry Point**
```cpp
int main() {
    int nRetCode = 0;
    
    HMODULE hModule = ::GetModuleHandle(nullptr);
    
    if (hModule != nullptr) {
        // Khởi tạo MFC (Microsoft Foundation Classes)
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else {
            const int SERVER_PORT = 5555;
            try {
                SocketHelper wsHelper;  // RAII: Tự động khởi tạo WinSock
                
                Server server(SERVER_PORT);
                server.run();  // Bắt đầu server (blocking call)
            }
            catch (const exception& e) {
                cerr << "ERROR: " << e.what() << endl;
                nRetCode = 1;
            }
        }
    }
    
    return nRetCode;
}
```

**Logic**:
- **MFC Initialization**: Trên Windows, cần khởi tạo MFC framework
- **SocketHelper**: RAII object, tự động khởi tạo WinSock khi tạo, cleanup khi hủy
- **Server::run()**: Blocking call - chạy mãi mãi cho đến khi process bị kill
- **Exception handling**: Bắt lỗi và trả về error code

**Tại sao dùng try-catch?**
- WSAStartup có thể fail
- Socket operations có thể throw exception
- Cần handle gracefully thay vì crash

---

#### **Server/Server/ClientSession.h**
**Mục đích**: Định nghĩa class xử lý từng client session

**Logic**:
```cpp
class ClientSession {
private:
    SOCKET clientSocket;  // Socket để giao tiếp với client cụ thể
    
public:
    ClientSession(SOCKET clientSocket);  // Constructor
    virtual ~ClientSession();             // Destructor
    void run();                           // Hàm chính xử lý client
};
```

**Tại sao cần class riêng?**
- Mỗi client có state riêng (socket, video stream, session ID)
- Dễ quản lý và maintain
- Có thể mở rộng thêm các thuộc tính khác (username, video file, etc.)

---

#### **Server/Server/ClientSession.cpp**
**Mục đích**: Implementation của ClientSession

##### **ClientSession::ClientSession() - Constructor**
```cpp
ClientSession::ClientSession(SOCKET clientSocket) 
    : clientSocket(clientSocket) 
{
    cout << "[Session " << clientSocket << "] Da duoc khoi tao\n";
}
```

**Logic**:
- Lưu `clientSocket` vào member variable
- Log thông tin session được tạo
- **clientSocket**: Socket descriptor từ accept(), dùng để gửi/nhận data với client

---

##### **ClientSession::~ClientSession() - Destructor**
```cpp
ClientSession::~ClientSession() {
    cout << "[Session " << clientSocket << "] Da duoc huy\n";
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);  // Đóng socket
    }
}
```

**Logic**:
- Log khi session bị hủy
- Đóng socket để giải phóng tài nguyên
- **Tại sao cần đóng?**: 
  - Giải phóng file descriptor
  - Thông báo cho client biết connection đã đóng
  - Tránh resource leak

---

##### **ClientSession::run() - Hàm Chính Xử Lý Client**
```cpp
void ClientSession::run() {
    vector<char> buffer(RECV_BUFFER_SIZE);  // Buffer 4096 bytes
    int bytesRec;
    
    try {
        // Vòng lặp nhận data từ client
        while ((bytesRec = recv(clientSocket, buffer.data(), 
                                buffer.size() - 1, 0)) > 0) {
            buffer[bytesRec] = '\0';  // Null-terminate
            string request(buffer.data());
            
            cout << "Client: " << request << endl;
        }
        
        // Xử lý kết thúc kết nối
        if (bytesRec == 0) {
            cout << "Client ngat ket noi!!!\n";
        }
        else {
            cerr << "[Session] ERROR: recv() that bai!!!\n";
        }
    }
    catch (const exception& e) {
        cerr << "[Session] ERROR: Ngoai le!!!\n";
    }
    
    delete this;  // Tự xóa object khi kết thúc
}
```

**Phân Tích Chi Tiết**:

1. **Buffer Allocation**:
   ```cpp
   vector<char> buffer(RECV_BUFFER_SIZE);  // 4096 bytes
   ```
   - **Tại sao dùng vector?**: 
     - Tự động quản lý memory
     - An toàn hơn raw array
     - Có thể resize nếu cần
   - **4096 bytes**: Đủ lớn để nhận hầu hết RTSP requests (thường < 256 bytes)

2. **recv() - Nhận Data**:
   ```cpp
   bytesRec = recv(clientSocket, buffer.data(), buffer.size() - 1, 0)
   ```
   - **Parameters**:
     - `clientSocket`: Socket để nhận data
     - `buffer.data()`: Con trỏ đến buffer
     - `buffer.size() - 1`: Nhận tối đa 4095 bytes (để lại 1 byte cho '\0')
     - `0`: Flags (0 = no special flags)
   - **Return values**:
     - `> 0`: Số bytes đã nhận
     - `= 0`: Client đã đóng connection (graceful shutdown)
     - `< 0`: Lỗi (SOCKET_ERROR)
   - **Blocking**: Hàm này sẽ chờ cho đến khi có data hoặc connection đóng

3. **Null-terminate String**:
   ```cpp
   buffer[bytesRec] = '\0';
   string request(buffer.data());
   ```
   - **Tại sao cần '\0'?**: C string cần null terminator
   - Chuyển sang `string` để dễ xử lý

4. **Vòng Lặp While**:
   - Tiếp tục nhận data cho đến khi:
     - Client đóng connection (`bytesRec == 0`)
     - Có lỗi xảy ra (`bytesRec < 0`)
   - **Tại sao vòng lặp?**: Client có thể gửi nhiều requests

5. **Xử Lý Kết Thúc**:
   ```cpp
   if (bytesRec == 0) {
       cout << "Client ngat ket noi!!!\n";
   }
   else {
       cerr << "[Session] ERROR: recv() that bai!!!\n";
   }
   ```
   - `bytesRec == 0`: Graceful shutdown (client gọi close())
   - `bytesRec < 0`: Lỗi (network error, timeout, etc.)

6. **Self-Deletion**:
   ```cpp
   delete this;
   ```
   - **Lưu ý**: Pattern này cần cẩn thận
   - Object tự xóa khi kết thúc
   - **Vấn đề**: Nếu có code khác đang dùng object này, sẽ gây crash
   - **Giải pháp tốt hơn**: Dùng smart pointer (shared_ptr) hoặc quản lý từ bên ngoài

**Luồng Hoạt Động**:
```
Client gửi: "SETUP movie.Mjpeg RTSP/1.0\r\n..."
    │
    ▼
recv() nhận được 256 bytes
    │
    ▼
Chuyển sang string: "SETUP movie.Mjpeg RTSP/1.0\r\n..."
    │
    ▼
In ra console: "Client: SETUP movie.Mjpeg RTSP/1.0\r\n..."
    │
    ▼
Tiếp tục chờ request tiếp theo...
```

**Vấn Đề Hiện Tại**:
- Code chỉ **nhận và in** requests, chưa **xử lý** RTSP protocol
- Chưa parse RTSP message
- Chưa gửi response về client
- Chưa xử lý RTP streaming

**Cần Bổ Sung**:
1. Parse RTSP requests (SETUP, PLAY, PAUSE, TEARDOWN)
2. Xử lý state machine (INIT → READY → PLAYING)
3. Gửi RTSP responses
4. Tạo RTP socket và gửi video packets
5. Quản lý VideoStream object

---

### 🖥️ 4. CLIENT IMPLEMENTATION

#### **src/client/include/RTSPClient.h** và **src/client/src/RTSPClient.cpp**
**Trạng thái**: Đang trống (placeholder)

**Mục đích dự kiến**: Class xử lý RTSP protocol phía client

**Logic cần implement**:

```cpp
class RTSPClient {
private:
    // State machine
    enum State { INIT, READY, PLAYING };
    State state;
    
    // Sockets
    SOCKET rtspSocket;  // TCP socket cho RTSP commands
    SOCKET rtpSocket;   // UDP socket cho RTP data
    
    // RTSP session info
    int sessionId;      // Session ID từ server
    int cseq;           // Sequence number cho requests
    
    // RTP info
    int frameNbr;       // Frame number hiện tại
    
    // Server info
    string serverIP;
    int serverPort;
    
public:
    RTSPClient(string serverIP, int port);
    ~RTSPClient();
    
    // RTSP commands
    bool setup(string videoFile, int rtpPort);
    bool play();
    bool pause();
    bool teardown();
    
    // RTP receiving
    void receiveRtpPackets();
    
    // Helper methods
    bool sendRtspRequest(string request);
    string receiveRtspResponse();
    void parseRtspResponse(string response);
};
```

**State Machine**:
```
INIT ──SETUP──► READY ──PLAY──► PLAYING
                      ◄──PAUSE──
                      ──TEARDOWN──► (disconnect)
```

---

#### **src/client/include/ClientWindow.h** và **src/client/src/ClientWindow.cpp**
**Trạng thái**: Đang trống (placeholder)

**Mục đích dự kiến**: Qt GUI window cho client

**Logic cần implement**:
- Buttons: Setup, Play, Pause, Teardown
- Video display area (QLabel hoặc QVideoWidget)
- Status bar
- Connect signals/slots để xử lý button clicks

---

### 📄 5. CÁC FILE KHÁC

#### **README.md**
Chỉ có mô tả ngắn gọn về project.

#### **doc/** Directory
Chứa reference implementations (Python, Java) và documentation về RTP/RTSP.

---

## 🔄 CÁC THUẬT TOÁN SỬ DỤNG

### 1. **Multi-Threading Pattern (Server)**
```
Main Thread:
  ├─► Tạo listening socket
  ├─► Bind và listen
  └─► Vòng lặp accept():
        ├─► Client 1 kết nối ──► Tạo Thread 1
        ├─► Client 2 kết nối ──► Tạo Thread 2
        └─► Client 3 kết nối ──► Tạo Thread 3

Thread 1 (ClientSession):
  └─► Nhận và xử lý requests từ Client 1

Thread 2 (ClientSession):
  └─► Nhận và xử lý requests từ Client 2
```

**Ưu điểm**:
- Phục vụ nhiều client đồng thời
- 1 client chậm không ảnh hưởng client khác

**Nhược điểm**:
- Tốn tài nguyên (memory, CPU) cho mỗi thread
- Cần quản lý synchronization nếu có shared data

---

### 2. **RAII Pattern (Resource Management)**
```cpp
{
    SocketHelper helper;  // Tự động khởi tạo WinSock
    // ... sử dụng sockets ...
}  // Tự động cleanup khi ra khỏi scope
```

**Ưu điểm**:
- Tự động quản lý resource
- Đảm bảo cleanup kể cả khi có exception
- Code sạch hơn, ít lỗi hơn

---

### 3. **State Machine (RTSP)**
```
INIT ──[SETUP OK]──► READY ──[PLAY]──► PLAYING
                          ◄──[PAUSE]──
                          ──[TEARDOWN]──► DISCONNECTED
```

**Logic**:
- Mỗi state chỉ chấp nhận một số commands nhất định
- Chuyển state dựa trên command và response
- Đảm bảo protocol được thực hiện đúng

---

### 4. **RTP Packetization Algorithm**
```
Video Frame (JPEG data, 50000 bytes)
    │
    ▼
Chia thành chunks (nếu cần)
    │
    ▼
Tạo RTP Header:
  - Version = 2
  - Payload Type = 26 (MJPEG)
  - Sequence Number = frameNbr
  - Timestamp = currentTime
  - SSRC = 0
    │
    ▼
Gắn Header + Payload
    │
    ▼
Gửi qua UDP socket
```

**Tại sao UDP cho RTP?**
- Video cần tốc độ cao
- Mất 1 vài packets không quan trọng bằng delay
- Client có thể xử lý lost packets (skip frame)

---

## 🔄 LUỒNG HOẠT ĐỘNG CỦA HỆ THỐNG

### **Luồng Kết Nối và Streaming**

#### **Bước 1: Server Khởi Động**
```
1. Khởi tạo WinSock (Windows) hoặc dùng POSIX sockets (Linux)
2. Tạo TCP socket
3. Bind vào port 5555
4. Listen cho kết nối
5. Chờ client kết nối (blocking)
```

#### **Bước 2: Client Kết Nối**
```
Client:
  1. Tạo TCP socket
  2. Connect đến server (IP: 5555)
  3. Gửi SETUP request:
     SETUP movie.Mjpeg RTSP/1.0
     CSeq: 1
     Transport: RTP/UDP; client_port=25000

Server:
  1. Accept connection
  2. Tạo ClientSession thread
  3. Parse SETUP request
  4. Tạo VideoStream object
  5. Generate session ID
  6. Gửi response:
     RTSP/1.0 200 OK
     CSeq: 1
     Session: 123456
```

#### **Bước 3: Client Yêu Cầu Play**
```
Client:
  1. Tạo UDP socket cho RTP (port 25000)
  2. Gửi PLAY request:
     PLAY movie.Mjpeg RTSP/1.0
     CSeq: 2
     Session: 123456

Server:
  1. Parse PLAY request
  2. Tạo UDP socket cho RTP
  3. Bắt đầu thread gửi RTP packets
  4. Gửi response:
     RTSP/1.0 200 OK
     CSeq: 2
     Session: 123456
```

#### **Bước 4: Server Gửi Video (RTP)**
```
Server Thread (sendRtp):
  while (playing) {
    1. Đọc frame tiếp theo từ video file
    2. Tạo RTP packet:
       - Header: version, payload type, seq num, timestamp
       - Payload: frame data (JPEG)
    3. Gửi qua UDP đến client (IP: clientIP, Port: 25000)
    4. Đợi 100ms (frame rate control)
  }
```

#### **Bước 5: Client Nhận và Hiển Thị**
```
Client Thread (receiveRtp):
  while (playing) {
    1. Nhận RTP packet từ UDP socket
    2. Parse RTP header:
       - Kiểm tra sequence number
       - Lấy timestamp
       - Lấy payload
    3. Decode JPEG frame
    4. Hiển thị trên GUI
    5. Cập nhật frame number
  }
```

#### **Bước 6: Client Pause/Teardown**
```
PAUSE:
  Client gửi: PAUSE request
  Server: Dừng thread gửi RTP, gửi response
  Client: Dừng thread nhận RTP

TEARDOWN:
  Client gửi: TEARDOWN request
  Server: Đóng RTP socket, cleanup, gửi response
  Client: Đóng sockets, cleanup, đóng GUI
```

---

## 📊 TÓM TẮT

### **Những Gì Đã Có**:
✅ Server cơ bản: Nhận kết nối, tạo thread cho mỗi client
✅ Cross-platform socket code (Windows/Linux)
✅ RAII pattern cho resource management
✅ Multi-threading architecture

### **Những Gì Còn Thiếu**:
❌ RTSP protocol implementation (parse requests, send responses)
❌ RTP packetization (encode/decode RTP packets)
❌ VideoStream class (đọc video file, extract frames)
❌ Client implementation (RTSP client, RTP receiver, GUI)
❌ State machine cho RTSP session
❌ Error handling đầy đủ

### **Hướng Phát Triển**:
1. Hoàn thiện RtpPacket class
2. Implement RTSP protocol handler
3. Implement VideoStream để đọc MJPEG file
4. Implement client với GUI
5. Thêm error handling và logging
6. Thêm authentication nếu cần
7. Hỗ trợ multiple video files

---

## 🎓 KẾT LUẬN

Dự án này là một **ứng dụng streaming video** sử dụng **RTSP/RTP protocols**, được thiết kế để:
- Học tập về network programming
- Hiểu cách thức hoạt động của video streaming
- Thực hành multi-threading và socket programming

Hiện tại, project mới có **phần server cơ bản** (nhận kết nối), còn thiếu nhiều thành phần quan trọng như RTSP/RTP handling và client implementation. Tuy nhiên, kiến trúc đã được thiết kế tốt với multi-threading và cross-platform support, tạo nền tảng vững chắc để phát triển tiếp.

