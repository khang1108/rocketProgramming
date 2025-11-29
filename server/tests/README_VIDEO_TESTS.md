# VideoStream Test Suite Documentation

## 📋 Tổng Quan

Test suite này kiểm tra class `VideoStream` - component chịu trách nhiệm đọc MJPEG video files trong RTSP/RTP streaming server.

**File được test:**
- `server/include/video/VideoStream.hpp`
- `server/src/video/VideoStream.cpp`

**Test file:**
- `server/tests/test_video_stream.cpp`

---

## 🎯 Mục Đích Test

VideoStream class quản lý việc đọc MJPEG file format:
- **MJPEG format**: Chuỗi các JPEG images
- **Mỗi frame**: `[5-byte length header][JPEG data]`
- **Header**: Big-endian 40-bit integer chứa frame length

### MJPEG File Structure

```
[Frame 0 length (5 bytes)][Frame 0 JPEG data (variable)]
[Frame 1 length (5 bytes)][Frame 1 JPEG data (variable)]
[Frame 2 length (5 bytes)][Frame 2 JPEG data (variable)]
...
```

### Frame Length Header (5 bytes)

```
Byte 0: Most significant byte
Byte 1: ...
Byte 2: ...
Byte 3: ...
Byte 4: Least significant byte
```

---

## 🧪 Test Categories

### 1. **Constructor Tests** (3 tests)
Kiểm tra việc mở file và validation:
- ✅ Constructor với file hợp lệ
- ✅ Constructor với file không tồn tại (should throw)
- ✅ Constructor với file rỗng (should throw)

### 2. **Frame Reading Tests** (3 tests)
Kiểm tra đọc frames:
- ✅ Đọc tất cả frames đúng kích thước
- ✅ `hasMoreFrames()` trả về `false` khi hết file
- ✅ `nextFrame()` throw exception khi đọc quá EOF

### 3. **Navigation Tests** (6 tests)
Kiểm tra di chuyển trong file:
- ✅ Frame number ban đầu là 0
- ✅ Frame number tăng sau mỗi lần đọc
- ✅ `rewind()` reset frame number về 0
- ✅ `hasMoreFrames()` là `true` sau rewind
- ✅ Có thể đọc lại từ đầu sau rewind
- ✅ Multiple rewinds hoạt động đúng

### 4. **Error Handling Tests** (3 tests)
Kiểm tra xử lý lỗi:
- ✅ Corrupt file (incomplete header) → throw exception
- ✅ Frame size quá lớn (>50MB) → throw exception
- ✅ Incomplete frame data → throw exception

### 5. **Boundary Condition Tests** (8 tests)
Kiểm tra các trường hợp biên:
- ✅ Single frame file
- ✅ Very small frame (10 bytes)
- ✅ Large frame (1MB)
- ✅ Many frames (100 frames)
- ✅ Variable frame sizes

### 6. **Loop Playback Simulation** (2 tests)
Mô phỏng chơi video lặp lại:
- ✅ Đọc 3 lần liên tiếp với rewind

---

## 🔨 Build & Run

### Using Makefile (Recommended)

```bash
# Build and run video stream test only
make video

# Build all tests
make all

# Run all tests
make test

# Clean build artifacts
make clean
```

### Manual Compilation

#### Linux/MinGW:

```bash
cd server/tests

g++ -std=c++17 -Wall -Wextra -pedantic \
    -I../include -I../../common/include \
    test_video_stream.cpp ../src/video/VideoStream.cpp \
    -o test_video_stream -pthread

./test_video_stream
```

#### Windows (MSVC):

```cmd
cd server\tests

cl /EHsc /std:c++17 \
   /I..\include /I..\..\common\include \
   test_video_stream.cpp ..\src\video\VideoStream.cpp \
   /Fe:test_video_stream.exe

test_video_stream.exe
```

---

## 📊 Test Output Example

```
╔══════════════════════════════════════════════════════════════════════╗
║                 VideoStream Class Test Suite                        ║
║                  MJPEG File Reader Testing                          ║
╚══════════════════════════════════════════════════════════════════════╝


======================================================================
📋 CONSTRUCTOR TESTS
======================================================================
✅ PASS: Constructor with valid file
✅ PASS: Constructor with non-existent file throws exception
✅ PASS: Constructor with empty file throws exception

======================================================================
📋 FRAME READING TESTS
======================================================================
✅ PASS: Read all frames correctly
✅ PASS: hasMoreFrames() is false at end
✅ PASS: nextFrame() at EOF throws exception

...

======================================================================
📊 TEST SUMMARY
======================================================================
Total Tests:  25
✅ Passed:     25
❌ Failed:     0
Success Rate: 100%
======================================================================
```

---

## 🧪 Test Implementation Details

### Mock MJPEG File Generator

Test suite tự động tạo mock MJPEG files để test:

```cpp
bool createMockMJPEGFile(const std::string& filename, 
                         int numFrames, 
                         size_t frameSize) {
    // Creates valid MJPEG file with:
    // - JPEG magic bytes (FF D8)
    // - Correct 5-byte headers
    // - Variable frame data
}
```

### Test Files Created

Các file test được tạo tự động và xóa sau mỗi test:
- `test_valid.mjpeg` - File hợp lệ
- `test_empty.mjpeg` - File rỗng
- `test_corrupt.mjpeg` - File corrupt (header không đủ)
- `test_reading.mjpeg` - Nhiều frames để test đọc
- `test_navigation.mjpeg` - Test rewind và navigation
- `test_loop.mjpeg` - Test loop playback

**Note**: Tất cả files được cleanup tự động sau test.

---

## 🔍 Test Coverage

### Methods Tested:
- ✅ `VideoStream(const std::string& filename)` - Constructor
- ✅ `~VideoStream()` - Destructor (implicit)
- ✅ `nextFrame()` - Đọc frame tiếp theo
- ✅ `hasMoreFrames()` - Check còn frames không
- ✅ `rewind()` - Quay về đầu file
- ✅ `getCurrentFrameNumber()` - Lấy số thứ tự frame

### Error Cases Tested:
- ✅ File không tồn tại
- ✅ File rỗng
- ✅ Header không đủ 5 bytes
- ✅ Frame size quá lớn (>50MB)
- ✅ Frame data không đủ (file kết thúc đột ngột)
- ✅ Đọc quá EOF

### Boundary Cases Tested:
- ✅ Single frame file
- ✅ Empty file
- ✅ Very small frames (10 bytes)
- ✅ Large frames (1MB)
- ✅ Many frames (100+)
- ✅ Variable frame sizes
- ✅ Loop playback (rewind multiple times)

---

## 📁 File Structure

```
server/
├── include/
│   └── video/
│       └── VideoStream.hpp         # Header file
├── src/
│   └── video/
│       └── VideoStream.cpp         # Implementation
└── tests/
    ├── test_video_stream.cpp       # Test suite (25 tests)
    ├── Makefile                    # Build automation
    └── README_VIDEO_TESTS.md       # This file
```

---

## 🔧 Dependencies

### Required:
- C++17 compiler (g++/clang++/MSVC)
- Standard C++ Library (fstream, iostream, vector)

### No External Dependencies:
- ❌ No OpenCV
- ❌ No libjpeg
- ❌ No FFmpeg
- ✅ Pure C++ standard library

**Why?** VideoStream chỉ đọc raw bytes từ file, không decode JPEG. Decoding được handle bởi client-side UI.

---

## 🎓 Usage Example (Server Context)

```cpp
// Server worker thread streaming video to client
void ServerWorker::streamVideo() {
    try {
        // Open video file
        VideoStream video("movie.Mjpeg");
        
        // Stream loop
        while (video.hasMoreFrames() && playing_) {
            // Read next frame (JPEG data)
            auto frameData = video.nextFrame();
            
            // Create RTP packets (using EncodingStrategy)
            Frame frame(frameData, seqNum_, timestamp_, ssrc_);
            auto packets = encodingContext_->encodeFrame(frame);
            
            // Send each packet via UDP
            for (auto& packet : packets) {
                udpSocket_->sendTo(packet.getPacketVector(), 
                                  clientIP_, 
                                  clientRTPPort_);
            }
            
            // Wait 40ms (25 fps)
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            
            seqNum_++;
            timestamp_ += 3600;
        }
        
        // Loop playback
        if (loopEnabled_) {
            video.rewind();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Video streaming error: " + std::string(e.what()));
    }
}
```

---

## 🐛 Debugging Failed Tests

### If test fails:

1. **Check compiler version:**
   ```bash
   g++ --version  # Should be >= 7.0 for C++17
   ```

2. **Check file permissions:**
   ```bash
   ls -la test_*.mjpeg  # Should be readable/writable
   ```

3. **Run with verbose output:**
   ```bash
   ./test_video_stream 2>&1 | tee test_output.log
   ```

4. **Check for file system issues:**
   - Disk full?
   - Write permissions in current directory?

---

## 📊 Performance Characteristics

### File Operations:
- **Open**: O(1) - Instant file handle acquisition
- **Read Frame**: O(n) where n = frame size
- **Rewind**: O(1) - Simple seek operation
- **hasMoreFrames**: O(1) - Peek next byte

### Memory Usage:
- **Per Frame**: Allocates `std::vector<uint8_t>` of frame size
- **Typical**: 10KB - 100KB per frame (JPEG compressed)
- **Max**: 50MB safety limit (configurable)

### Typical Frame Sizes:
- SD (640x480): ~10-30 KB
- HD (1280x720): ~50-100 KB
- FHD (1920x1080): ~100-200 KB

---

## ✅ Success Criteria

All tests should **PASS** with:
- ✅ 25/25 tests passed
- ✅ 100% success rate
- ✅ No memory leaks (valgrind clean)
- ✅ No warnings (compile with -Wall -Wextra)

### Expected Output:

```
Total Tests:  25
✅ Passed:     25
❌ Failed:     0
Success Rate: 100%
```

---

## 🔗 Related Components

VideoStream is used by:
- **ServerWorker** - Reads frames for streaming
- **EncodingStrategy** - Fragments frames into RTP packets
- **RTPPacketBuilder** - Builds packets from frame data

Dependencies:
- None (standalone component)

---

## 📝 Notes

1. **JPEG Validation**: Test chỉ kiểm tra JPEG magic bytes (FF D8), không decode full JPEG
2. **Cross-Platform**: Test hoạt động trên Linux, Windows, macOS
3. **No Network**: Test không cần network, chỉ test file I/O
4. **Auto Cleanup**: Test files được xóa tự động sau mỗi test
5. **Thread-Safe**: VideoStream KHÔNG thread-safe (mỗi thread cần instance riêng)

---

## 🚀 Next Steps

Sau khi VideoStream pass tất cả tests:

1. ✅ Integrate với ServerWorker
2. ✅ Test với real MJPEG file (movie.Mjpeg)
3. ✅ Test streaming performance
4. ✅ Add frame rate control (Timer class)
5. ✅ Add loop playback option

---

## 📖 References

- MJPEG Format: [Wikipedia - Motion JPEG](https://en.wikipedia.org/wiki/Motion_JPEG)
- RTP Protocol: RFC 3550
- RTSP Protocol: RFC 2326
- JPEG Format: [JPEG File Interchange Format](https://www.w3.org/Graphics/JPEG/)

---

**Last Updated:** 2024-11-29  
**Test Suite Version:** 1.0  
**Status:** ✅ All tests passing

