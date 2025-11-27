# RTP PACKET SYSTEM - DESIGN PATTERNS DOCUMENTATION

## 📋 OVERVIEW

Hệ thống RTP Packet implementation sử dụng **3 Design Patterns** chính:
1. **Builder Pattern** - Xây dựng RTP packets
2. **Strategy Pattern** - Lựa chọn encoding algorithm (SD/HD)
3. **Singleton Pattern** - Logger, Config, Metrics

---

## 🎨 DESIGN PATTERNS

### 1. Builder Pattern (RTPPacketBuilder)

**Mục đích**: Tách rời quá trình xây dựng packet phức tạp khỏi class RTPPacket.

**Lý do sử dụng**:
- RTP header có 12 bytes với nhiều trường phức tạp (bit-level manipulation)
- Cần validate dữ liệu trước khi build packet final
- Fluent interface (method chaining) giúp code dễ đọc
- Dễ extend cho các loại packet khác (RTCP, custom extensions)

**Files**:
- `server/include/patterns/Builder.hpp` - Base builder interface
- `server/include/rtp/RTPPacketBuilder.hpp` - Concrete builder cho RTP

**Example Usage**:
```cpp
// Simple SD packet
RTPPacket packet = RTPPacketBuilder()
    .setPayloadType(26)
    .setSequenceNumber(100)
    .setTimestamp(RTPPacket::getCurrentTimestamp())
    .setSSRC(12345)
    .setMarker(0)
    .setPayload(frameData.data(), frameData.size())
    .build();

// HD fragmented packet
RTPPacket fragment = RTPPacketBuilder()
    .setSequenceNumber(seqNum++)
    .setTimestamp(sameTimestamp)  // Same timestamp cho all fragments
    .setSSRC(serverID)
    .setMarker(isLastFragment ? 1 : 0)
    .setPayload(chunkData, chunkSize)
    .build();
```

---

### 2. Strategy Pattern (EncodingStrategy)

**Mục đích**: Chọn thuật toán encoding phù hợp runtime (SD vs HD).

**Lý do sử dụng**:
- SD video (<1400 bytes) → single packet per frame
- HD video (>1400 bytes) → fragmentation needed
- Dễ thêm strategies mới (4K, adaptive bitrate, error correction)
- Testable: test từng strategy độc lập

**Files**:
- `server/include/patterns/Strategy.hpp` - Base strategy interface
- `server/include/rtp/EncodingStrategy.hpp` - Concrete strategies

**Strategies**:

#### SDEncodingStrategy
- **Input**: Frame ≤ 1400 bytes
- **Output**: 1 RTP packet
- **Marker bit**: Always 1 (complete frame)
- **Use case**: SD video (480p, 720p với low bitrate)

#### HDEncodingStrategy
- **Input**: Frame > 1400 bytes
- **Output**: Multiple RTP packets (fragments)
- **Marker bit**: 0 for all packets except last (1)
- **Use case**: HD video (1080p, 4K)
- **Algorithm**: 
  ```
  numPackets = ceil(frameSize / 1400)
  for each chunk:
      - Extract ≤1400 bytes
      - Same timestamp for all
      - Marker=1 only for last
      - Sequence number increments
  ```

**Example Usage**:
```cpp
// Auto-detection (recommended)
EncodingContext context;
Frame frame(jpegData, seqNum, timestamp, ssrc);
std::vector<RTPPacket> packets = context.encodeFrame(frame);
// Auto chọn SD hoặc HD strategy dựa trên frame size

// Manual strategy
context.setAutoDetect(false);
context.setStrategy(std::make_unique<HDEncodingStrategy>());
packets = context.encodeFrame(frame);  // Force HD
```

---

### 3. Singleton Pattern (Logger, Config, Metrics)

**Mục đích**: Đảm bảo chỉ có 1 instance duy nhất trong toàn app.

**Lý do sử dụng**:
- **Logger**: Tất cả modules cùng log vào một file/console
- **Config**: Load config một lần, dùng ở mọi nơi
- **Metrics**: Thu thập statistics từ nhiều modules

**Files**:
- `server/include/patterns/Singleton.hpp` - Base singleton interface

**Example Usage**:
```cpp
// Logger singleton
class Logger : public Singleton<Logger> {
    friend class Singleton<Logger>;
private:
    Logger() { /* private */ }
public:
    void log(const std::string& msg) { /* ... */ }
};

// Usage
Logger::getInstance().log("Server started");
Logger::getInstance().log("Client connected");

// Config singleton (with initialization)
class Config : public SingletonWithInit<Config> {
    friend class SingletonWithInit<Config>;
private:
    Config(const std::string& file) { /* load config */ }
public:
    static void initialize(const std::string& file) {
        std::call_once(initFlag_, [&]() {
            instance_.reset(new Config(file));
        });
    }
};

// Usage
Config::initialize("server.conf");
auto& config = Config::getInstance();
int port = config.getPort();
```

---

## 📦 CLASS STRUCTURE

### RTPPacket (Data Class)
**Purpose**: Simple data container cho RTP packet data
**Responsibilities**:
- Store header fields (version, payload type, seq, timestamp, ssrc, etc.)
- Store payload (video frame bytes)
- Encode: pack fields vào 12-byte header (network byte order)
- Decode: parse 12-byte header thành fields
- Getters/Setters
- Validation

**NOT responsible for**:
- Building packets (→ RTPPacketBuilder's job)
- Choosing encoding strategy (→ EncodingStrategy's job)

---

### RTPPacketBuilder (Builder Pattern)
**Purpose**: Construct RTP packets với fluent interface
**Responsibilities**:
- Provide method chaining interface (setXXX methods)
- Validate required fields before build
- Build final RTPPacket object
- Call RTPPacket.encode() để tạo raw header bytes

**Key Methods**:
```cpp
setVersion(uint8_t)        // Default: 2
setPadding(uint8_t)        // Default: 0
setExtension(uint8_t)      // Default: 0
setCC(uint8_t)             // Default: 0
setMarker(uint8_t)         // IMPORTANT for HD fragmentation
setPayloadType(uint8_t)    // Default: 26 (MJPEG)
setSequenceNumber(uint16_t) // REQUIRED
setTimestamp(uint32_t)     // REQUIRED
setSSRC(uint32_t)          // REQUIRED
setPayload(...)            // REQUIRED
build()                    // Build final packet
reset()                    // Reuse builder
```

---

### EncodingStrategy Hierarchy

```
Strategy<Frame, std::vector<RTPPacket>>  [Base Interface]
    │
    ├── SDEncodingStrategy  [Concrete - Single Packet]
    │       execute(Frame) → 1 RTP packet
    │
    └── HDEncodingStrategy  [Concrete - Fragmentation]
            execute(Frame) → N RTP packets

Context<Frame, std::vector<RTPPacket>>  [Base Context]
    │
    └── EncodingContext  [Concrete Context]
            - Auto-detection based on frame size
            - Manual strategy selection
            - Statistics/info methods
```

**Key Methods**:
```cpp
// SDEncodingStrategy
std::vector<RTPPacket> execute(const Frame& frame)
    → returns 1 packet

// HDEncodingStrategy
std::vector<RTPPacket> execute(const Frame& frame)
    → returns multiple packets (fragments)

// EncodingContext
void setAutoDetect(bool enable)
void setStrategy(std::unique_ptr<EncodingStrategy>)
std::vector<RTPPacket> encodeFrame(const Frame&)
std::string getEncodingInfo(const Frame&)  // Statistics
```

---

## 🔄 WORKFLOW

### Server Side - Encoding & Sending

```cpp
// 1. Initialize context
EncodingContext encoder;

// 2. Read video frame
VideoStream videoStream("movie.Mjpeg");
std::vector<uint8_t> frameData = videoStream.nextFrame();

// 3. Create Frame object
uint16_t seqNum = currentSequenceNumber;
uint32_t timestamp = RTPPacket::getCurrentTimestamp();
uint32_t ssrc = 12345;  // Server ID
Frame frame(frameData, seqNum, timestamp, ssrc);

// 4. Encode frame (auto-detect SD/HD)
std::vector<RTPPacket> packets = encoder.encodeFrame(frame);

// 5. Send packets over UDP
for (const auto& packet : packets) {
    auto data = packet.getPacketVector();
    socket->sendTo(data.data(), data.size(), clientAddr, clientPort);
    
    currentSequenceNumber++;  // Increment for next packet
    
    // Frame rate control: 25 fps = 40ms per frame
    std::this_thread::sleep_for(std::chrono::milliseconds(40 / packets.size()));
}
```

### Client Side - Receiving & Decoding

```cpp
// 1. Receive RTP packet from UDP
uint8_t buffer[2000];
int bytesReceived = socket->receiveFrom(buffer, sizeof(buffer));

// 2. Decode packet
RTPPacket packet(buffer, bytesReceived);

// 3. Validate packet
if (!packet.validate()) {
    std::cerr << "Invalid packet" << std::endl;
    continue;
}

// 4. Extract info
uint16_t seqNum = packet.getSequenceNumber();
uint32_t timestamp = packet.getTimestamp();
uint8_t marker = packet.getMarker();
const auto& payload = packet.getPayload();

// 5. Handle fragmentation
if (marker == 0) {
    // Not last fragment, buffer it
    frameReassembler->addFragment(packet);
} else {
    // Last fragment or complete frame
    if (frameReassembler->hasFragments()) {
        // Reassemble all fragments
        std::vector<uint8_t> completeFrame = frameReassembler->getCompleteFrame();
        displayFrame(completeFrame);
    } else {
        // Single packet frame (SD)
        displayFrame(payload);
    }
}
```

---

## 📐 RTP HEADER STRUCTURE

```
Byte 0: [V=2][P][X][CC]
Byte 1: [M][PT=26]
Bytes 2-3: Sequence Number (16-bit big-endian)
Bytes 4-7: Timestamp (32-bit big-endian)
Bytes 8-11: SSRC (32-bit big-endian)
```

### Bit Manipulation Examples

**Encoding (pack fields → bytes)**:
```cpp
// Byte 0: [V(2) | P(1) | X(1) | CC(4)]
header[0] = (version << 6) | (padding << 5) | (extension << 4) | cc;

// Byte 1: [M(1) | PT(7)]
header[1] = (marker << 7) | payloadType;

// Bytes 2-3: Sequence Number (big-endian)
uint16_t seqNet = htons(sequenceNumber);
header[2] = (seqNet >> 8) & 0xFF;
header[3] = seqNet & 0xFF;

// Bytes 4-7: Timestamp (big-endian)
uint32_t tsNet = htonl(timestamp);
header[4] = (tsNet >> 24) & 0xFF;
header[5] = (tsNet >> 16) & 0xFF;
header[6] = (tsNet >> 8) & 0xFF;
header[7] = tsNet & 0xFF;
```

**Decoding (bytes → fields)**:
```cpp
// Byte 0: Extract version, padding, extension, CC
version = (header[0] >> 6) & 0x03;    // Bits 7-6
padding = (header[0] >> 5) & 0x01;    // Bit 5
extension = (header[0] >> 4) & 0x01;  // Bit 4
cc = header[0] & 0x0F;                 // Bits 3-0

// Byte 1: Extract marker, payload type
marker = (header[1] >> 7) & 0x01;     // Bit 7
payloadType = header[1] & 0x7F;       // Bits 6-0

// Bytes 2-3: Sequence number
sequenceNumber = (header[2] << 8) | header[3];
sequenceNumber = ntohs(sequenceNumber);

// Bytes 4-7: Timestamp
timestamp = (header[4] << 24) | (header[5] << 16) | 
            (header[6] << 8) | header[7];
timestamp = ntohl(timestamp);
```

---

## 🧪 TESTING EXAMPLES

### Test RTPPacketBuilder
```cpp
void testBuilder() {
    std::vector<uint8_t> testData = {1, 2, 3, 4, 5};
    
    RTPPacket packet = RTPPacketBuilder()
        .setPayloadType(26)
        .setSequenceNumber(100)
        .setTimestamp(123456)
        .setSSRC(12345)
        .setMarker(1)
        .setPayload(testData)
        .build();
    
    assert(packet.getPayloadType() == 26);
    assert(packet.getSequenceNumber() == 100);
    assert(packet.getMarker() == 1);
    assert(packet.getPayloadSize() == 5);
}
```

### Test SDEncodingStrategy
```cpp
void testSDStrategy() {
    SDEncodingStrategy strategy;
    std::vector<uint8_t> smallFrame(800, 0xFF);  // 800 bytes
    Frame frame(smallFrame, 1, 1000, 12345);
    
    auto packets = strategy.execute(frame);
    
    assert(packets.size() == 1);
    assert(packets[0].getMarker() == 1);
    assert(packets[0].getPayloadSize() == 800);
}
```

### Test HDEncodingStrategy
```cpp
void testHDStrategy() {
    HDEncodingStrategy strategy;
    std::vector<uint8_t> largeFrame(5000, 0xFF);  // 5000 bytes
    Frame frame(largeFrame, 1, 1000, 12345);
    
    auto packets = strategy.execute(frame);
    
    // Expected: ceil(5000 / 1400) = 4 packets
    assert(packets.size() == 4);
    
    // Check marker bits
    assert(packets[0].getMarker() == 0);
    assert(packets[1].getMarker() == 0);
    assert(packets[2].getMarker() == 0);
    assert(packets[3].getMarker() == 1);  // Last fragment
    
    // Check timestamps (all same)
    uint32_t ts = packets[0].getTimestamp();
    for (const auto& p : packets) {
        assert(p.getTimestamp() == ts);
    }
    
    // Check sequence numbers (incremental)
    for (size_t i = 1; i < packets.size(); ++i) {
        assert(packets[i].getSequenceNumber() == 
               packets[i-1].getSequenceNumber() + 1);
    }
}
```

---

## 🎯 KEY TAKEAWAYS

### Builder Pattern Benefits
✅ Fluent interface (easy to use)
✅ Validation before build
✅ Separates construction from representation
✅ Easy to extend with new packet types

### Strategy Pattern Benefits
✅ Runtime algorithm selection
✅ Easy to add new strategies (4K, adaptive, etc.)
✅ Testable in isolation
✅ Client code doesn't know implementation details

### Singleton Pattern Benefits
✅ Single global instance (Logger, Config)
✅ Lazy initialization
✅ Thread-safe (C++11 magic statics)
✅ Global access point

### Design Principles Applied
- **Single Responsibility**: Mỗi class có 1 nhiệm vụ rõ ràng
- **Open/Closed**: Mở cho extension (new strategies), đóng cho modification
- **Dependency Inversion**: Depend on abstractions (Strategy interface), not concrete classes
- **Interface Segregation**: Builder, Strategy có interfaces nhỏ, focused

---

## 📚 FILES SUMMARY

```
server/include/
├── patterns/
│   ├── Builder.hpp          ✅ Base builder interface
│   ├── Strategy.hpp         ✅ Base strategy + context interface
│   └── Singleton.hpp        ✅ Singleton pattern template
│
├── rtp/
│   ├── RTPPacket.hpp        ✅ Data class (12-byte header + payload)
│   ├── RTPPacketBuilder.hpp ✅ Builder pattern implementation
│   └── EncodingStrategy.hpp ✅ SD/HD strategies + context
│
└── ...
```

**Status**: ✅ All pattern interfaces và RTP classes defined với full documentation

**Next Steps** (Implementation .cpp files):
1. `RTPPacket.cpp` - Implement encode(), decode(), utility methods
2. `Logger.cpp`, `Config.cpp` - Implement singletons
3. `ServerWorker.cpp` - Use EncodingContext để encode & send
4. `RTPReceiver.cpp` - Use RTPPacket để decode & reassemble
