# RocketProgramming - Real-Time Video Streaming System

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16+-green.svg)](https://cmake.org/)
[![Qt6](https://img.shields.io/badge/Qt-6.0+-brightgreen.svg)](https://www.qt.io/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Documentation

| Document                      | Description                                      | Language   |
| ----------------------------- | ------------------------------------------------ | ---------- |
| **[README.md](README.md)** | Architecture, design patterns, technical details | English    |
| **[report/](report/)**     | Academic report (LaTeX)                          | Vietnamese |

---

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

### 3. Getting Started

#### Quick Setup (Recommended)

The automated setup script handles everything for you - from dependency installation to project compilation.

##### Step 1: Clone the Repository

```bash
git clone https://github.com/khang1108/rocketProgramming.git
cd rocketProgramming
```

##### Step 2: Run Setup Script

**Linux / macOS:**

```bash
python3 setup.py
```

**Windows:**

```cmd
python setup.py
```

*Or simply double-click `setup.py` in File Explorer*

##### What the Setup Script Does:

1. Detects your operating system automatically
2. ✓ Checks if Qt6 is installed on your system
3. ✓ Provides step-by-step installation guide if Qt6 is missing
4. ✓ Configures CMake build system
5. ✓ Compiles server and client binaries
6. ✓ Shows you how to run the applications

##### Step 3: Run the Applications

After successful setup, executables are located in `build/bin/`:

**Start the Server:**

```bash
# Navigate to build directory
cd build/bin

# Launch server (port 8554)
./server 8554
```

**Start the Client (in a new terminal):**

```bash
# Navigate to build directory
cd build/bin

# Connect to server
# Syntax: ./client <server_ip> <server_port> <video_file>
./client localhost 8554 movie.Mjpeg
```

**Client GUI Controls:**

- `SETUP` - Initialize RTSP session
- `PLAY` - Start/resume video playback
- `PAUSE` - Pause video
- `TEARDOWN` - End session and close connection
- Timeline slider - Navigate through video (seek)

---

#### 🔧 Manual Setup (Advanced Users)

If you prefer manual installation or need custom configuration:

##### Prerequisites by Platform

**Linux (Ubuntu/Debian):**

```bash
sudo apt update
sudo apt install -y build-essential cmake g++ \
                    qt6-base-dev qt6-multimedia-dev
```

**Linux (Arch):**

```bash
sudo pacman -Syu
sudo pacman -S base-devel cmake qt6-base qt6-multimedia
```

**Windows (MSYS2 - Recommended):**

1. Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/)
2. Open **MSYS2 MINGW64** terminal (not MSYS2 MSYS!)
3. Install dependencies:

```bash
pacman -Syu  # Update package database
pacman -S mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-qt6-multimedia \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-gcc
```

*Alternative:* Qt.io installer (3-5 GB download, requires Qt account)

**macOS:**

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install qt@6 cmake
```

##### Build Process

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Compile (parallel build for speed)
# Linux/macOS:
make -j$(nproc)

# Windows (MSYS2):
ninja
```

##### Run Binaries

```bash
# Server
./bin/server <port>
# Example: ./bin/server 8554

# Client
./bin/client <server_ip> <port> <video_file>
# Example: ./bin/client 192.168.1.100 8554 movie.Mjpeg
```

---

#### Troubleshooting

**Problem: `Qt6 not found` during CMake configuration**

**Solution:**

- **Linux**: Install `qt6-base-dev` and `qt6-multimedia-dev` packages
- **Windows**: Make sure you opened **MSYS2 MINGW64** terminal (not MSYS2 MSYS)
- **macOS**: Run `export CMAKE_PREFIX_PATH=$(brew --prefix qt@6)`

---

**Problem: `command not found: python3` or `python`**

**Solution:**

- Ensure Python 3.x is installed: [python.org/downloads](https://www.python.org/downloads/)
- On Windows, try `py setup.py` instead

---

**Problem: Client shows "Connection refused"**

**Solution:**

1. Check if server is running: `./server 8554`
2. Verify server IP and port match client command
3. Ensure firewall allows port 8554 (TCP) and 25000 (UDP)
4. If running on different machines, use server's actual IP (not `localhost`)

---

**Problem: Video playback is choppy or laggy**

**Solution:**

- Check network quality (packet loss shown in client UI)
- Increase prebuffer size (edit `PREBUFFER_MIN` in `client/include/buffer/FrameBuffer.hpp`)
- Ensure no other heavy network applications are running

---

**Problem: `CMake Error: ... does not appear to contain CMakeLists.txt`**

**Solution:**

- Make sure you're in the project root directory: `cd rocketProgramming`
- Run `ls` to verify `CMakeLists.txt` exists

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
