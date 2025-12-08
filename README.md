# RocketProgramming - Real-Time Video Streaming System
## 1. Overview
**RocketProgramming** is a high-performance, cross-platform **real-time video streaming system** that implements industry-standard protocols (RTSP/RTP) for delivering video content over IP networks. Built from scratch in modern C++17, this project demonstrates professional software engineering practices through comprehensive design patterns, robust error handling, and platform-agnostic architecture.

### 2. What Does It Do?

The system enables **live video streaming** from a server to multiple clients over a network, similar to how YouTube, Netflix, or video conferencing applications work. The server reads MJPEG video files and transmits them in real-time using:

- **RTSP (Real-Time Streaming Protocol)** - For session control (setup, play, pause, teardown)
- **RTP (Real-Time Transport Protocol)** - For efficient video packet delivery

### 2. Key Features

#### **Cross-Platform Compatibility**
- **Linux**: Native POSIX sockets
- **Windows**: WinSock2 API
- Single codebase with platform-specific optimizations

#### **Design Pattern Implementation**
- **Strategy Pattern** - Adaptive encoding (SD/HD with automatic fragmentation)
- **Builder Pattern** - Clean RTP packet construction
- **Observer Pattern** - Frame buffer notifications
- **State Pattern** - RTSP client state machine
- **Singleton Pattern** - Logger and configuration management

#### **Performance Features**
- **Multi-threaded Architecture** - Concurrent client handling
- **Adaptive Encoding** - Auto-detects frame size and fragments large frames
- **Frame Buffer** - Producer-consumer pattern for smooth playback
- **Precision Timing** - Maintains 25 fps with drift correction
- **Packet Loss Tracking** - Real-time statistics and monitoring

#### **Robust Implementation**
- **RAII Pattern** - Automatic resource management
- **Move Semantics** - Efficient memory handling
- **Exception Safety** - Comprehensive error handling
- **Cross-platform Sockets** - Unified abstraction layer

### 3. How to Run

#### 1. Prerequisites

##### Linux (Ubuntu/Debian)
```bash
sudo apt install build-essential cmake g++
```
##### Linux (Arch)
```bash
sudo pacman -Sy base-devel cmake
```
##### macOS
```bash
brew install cmake**Optional (for video display):**
brew install opencv---
```
##### Linux
```bash
sudo apt install libopencv-dev    # Ubuntu/Debian
sudo pacman -Sy opencv             # Arch
```

#### 2. Clone & Build
```bash
# Clone repository
git clone https://github.com/khang1108/rocketProgramming.git
cd rocketProgramming

# Build project
mkdir build && cd build
cmake ..
make -j$(nproc) -9 <PID>**OpenCV not found:**
# Install OpenCV (see Prerequisites)
# Or project will use terminal mode automatically
```

#### 3. Run Server
```bash
# Start server on port 8554
./bin/server 8554
```

#### 4. Run Client
```bash
cd rocketProgramming/build

# Connect to server
./bin/client localhost 8554 movie.Mjpeg
```

### 4. Folder Structure
```bash
rocketProgramming/
├── .gitignore
├── CMakeLists.txt
├── README.md
├── LICENSE
├── details.md
├── doc/
├── images/
│
├── common/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── network/
│   │   │   ├── Socket.hpp
│   │   │   └── RTSPMessage.hpp
│   │   ├── patterns/
│   │   │   ├── Strategy.hpp
│   │   │   ├── Builder.hpp
│   │   │   ├── Observer.hpp
│   │   │   ├── Singleton.hpp
│   │   │   └── State.hpp
│   │   └── utils/
│   │       ├── Logger.hpp
│   │       ├── Config.hpp
│   │       ├── Timer.hpp
│   │       └── Metrics.hpp
│   └── src/
│       ├── network/
│       │   └── Socket.cpp
│       └── utils/
│           ├── Logger.cpp
│           └── Config.cpp
│
├── server/
│   ├── CMakeLists.txt
│   ├── config/
│   │   └── server.conf
│   ├── videos/
│   │   └── *.Mjpeg
│   ├── include/
│   │   ├── network/
│   │   │   ├── RTSPServer.hpp
│   │   │   └── ServerWorker.hpp
│   │   ├── rtp/
│   │   │   ├── RTPPacket.hpp
│   │   │   ├── RTPPacketBuilder.hpp
│   │   │   └── EncodingStrategy.hpp
│   │   └── video/
│   │       └── VideoStream.hpp
│   ├── src/
│   │   ├── main.cpp
│   │   ├── network/
│   │   │   ├── RTSPServer.cpp
│   │   │   └── ServerWorker.cpp
│   │   ├── rtp/
│   │   │   ├── RTPPacket.cpp
│   │   │   ├── RTPPacketBuilder.cpp
│   │   │   └── EncodingStrategy.cpp
│   │   └── video/
│   │       └── VideoStream.cpp
│   └── tests/
│       ├── Makefile
│       ├── test_rtp_packet.cpp
│       ├── test_encoding_strategy.cpp
│       └── test_video_stream.cpp
│
└── client/
    ├── CMakeLists.txt
    ├── config/
    │   └── client.conf
    ├── include/
    │   ├── network/
    │   │   └── RTSPClient.hpp
    │   ├── rtp/
    │   │   ├── RTPPacket.hpp
    │   │   ├── RTPReceiver.hpp
    │   │   └── FrameReassembler.hpp
    │   ├── buffer/
    │   │   ├── FrameBuffer.hpp
    │   │   └── BufferObserver.hpp
    │   ├── state/
    │   │   ├── ClientState.hpp
    │   │   └── ConcreteStates.hpp
    │   └── ui/
    │       ├── ClientUI.hpp
    │       └── FrameDisplay.hpp
    └── src/
        ├── main.cpp
        ├── network/
        │   └── RTSPClient.cpp
        ├── rtp/
        │   ├── RTPReceiver.cpp
        │   └── FrameReassembler.cpp
        ├── buffer/
        │   └── FrameBuffer.cpp
        ├── state/
        │   └── ClientState.cpp
        └── ui/
            ├── ClientUI.cpp
            └── FrameDisplay.cpp
```
### 5. Architecture
```bash
┌─────────────────────────────────────────────────────────────┐
│                    DEPLOYMENT DIAGRAM                       │
└─────────────────────────────────────────────────────────────┘

Machine 1 (Server):                  Machine 2 (Client):
┌──────────────────────┐            ┌──────────────────────┐
│   SERVER PROCESS     │            │   CLIENT PROCESS     │
│   (./server 8554)    │            │   (./client ...)     │
│                      │            │                      │
│  ┌────────────────┐  │            │  ┌────────────────┐  │
│  │ RTSPServer     │  │◄──RTSP────►│  │ RTSPClient     │  │
│  │ (TCP:8554)     │  │  (TCP)     │  │ (State Pattern)│  │
│  └────────────────┘  │            │  └────────────────┘  │
│          │           │            │          │           │
│          ▼           │            │          ▼           │
│  ┌────────────────┐  │            │  ┌────────────────┐  │
│  │ VideoStream    │  │            │  │ FrameBuffer    │  │
│  │ (Read MJPEG)   │  │            │  │ (Observer)     │  │
│  └────────────────┘  │            │  └────────────────┘  │
│          │           │            │          │           │
│          ▼           │            │          ▼           │
│  ┌────────────────┐  │            │  ┌────────────────┐  │
│  │ EncodingCtx    │  │            │  │ FrameDisplay   │  │
│  │ (Strategy)     │  │            │  │ (UI)           │  │
│  └────────────────┘  │            │  └────────────────┘  │
│          │           │            │          ▲           │
│          ▼           │            │          │           │
│  ┌────────────────┐  │            │  ┌────────────────┐  │
│  │ RTPPacketBuilder│ │            │  │FrameReassembler│  │
│  │ (Builder)       │ │            │  │                │  │
│  └────────────────┘  │            │  └────────────────┘  │
│          │           │            │          ▲           │
│          ▼           │            │          │           │
│  ┌────────────────┐  │            │  ┌────────────────┐  │
│  │ UDP Socket     │  │═══RTP═════►│  │ RTPReceiver    │  │
│  │ (Send packets) │  │  (UDP)     │  │ (UDP:25000)    │  │
│  └────────────────┘  │            │  └────────────────┘  │
└──────────────────────┘            └──────────────────────┘
```

### 6. RTSP PROTOCOL LOGIC

#### RTSP Message Format
```bash
RTSP Request Format:
┌────────────────────────────────────────────────┐
│ METHOD FILE RTSP/1.0\r\n                       │ ← Request line
│ CSeq: <sequence_number>\r\n                    │ ← Mandatory header
│ [Additional headers]\r\n                       │ ← Optional headers
│ \r\n                                           │ ← Empty line (end)
└────────────────────────────────────────────────┘

RTSP Response Format:
┌────────────────────────────────────────────────┐
│ RTSP/1.0 <status_code> <reason>\r\n            │ ← Status line
│ CSeq: <sequence_number>\r\n                    │ ← Echo CSeq
│ [Additional headers]\r\n                       │ ← Headers
│ \r\n                                           │ ← Empty line (end)
└────────────────────────────────────────────────┘
```