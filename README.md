# rocketProgramming
rocketProgramming is a Realtime Streaming Video application in which a server sends a video to a client in an end system. All of things in this project are built in C++ and Socket STL. This application use two **Protocols** *TCP & UDP* to communicate and send packets to other. 

# Folder Structure
```bash
rocketProgramming/
├── README.md
├── CMakeLists.txt (root - optional)
├── docs/
│   └── report.pdf
│
├── server/                          # ← SERVER EXECUTABLE
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── patterns/
│   │   │   ├── Strategy.hpp        # Encoding strategies
│   │   │   ├── Builder.hpp         # RTP packet builder
│   │   │   └── Singleton.hpp       # Logger, Config
│   │   ├── network/
│   │   │   ├── Socket.hpp          # Socket wrapper
│   │   │   ├── RTSPServer.hpp      # RTSP protocol handler
│   │   │   └── ServerWorker.hpp    # Per-client handler
│   │   ├── rtp/
│   │   │   ├── RTPPacket.hpp       # RTP packet structure
│   │   │   ├── RTPPacketBuilder.hpp # Builder pattern
│   │   │   └── EncodingStrategy.hpp # SD/HD strategies
│   │   ├── video/
│   │   │   └── VideoStream.hpp     # MJPEG file reader
│   │   └── utils/
│   │       ├── Logger.hpp          # Singleton logger
│   │       ├── Config.hpp          # Server config
│   │       └── Timer.hpp           # Timing utilities
│   ├── src/
│   │   ├── patterns/
│   │   ├── network/
│   │   ├── rtp/
│   │   ├── video/
│   │   ├── utils/
│   │   └── main.cpp                # Server entry point
│   ├── config/
│   │   └── server.conf
│   └── videos/
│       ├── movie.Mjpeg
│       ├── hd720p.Mjpeg
│       └── hd1080p.Mjpeg
│
└── client/                          # ← CLIENT EXECUTABLE
    ├── CMakeLists.txt
    ├── include/
    │   ├── patterns/
    │   │   ├── State.hpp           # State pattern base
    │   │   ├── Command.hpp         # RTSP commands
    │   │   ├── Observer.hpp        # Buffer observers
    │   │   └── Singleton.hpp       # Logger, Config
    │   ├── network/
    │   │   ├── Socket.hpp          # Socket wrapper
    │   │   └── RTSPClient.hpp      # RTSP protocol
    │   ├── rtp/
    │   │   ├── RTPPacket.hpp       # RTP packet structure
    │   │   ├── RTPReceiver.hpp     # Receive RTP packets
    │   │   └── FrameReassembler.hpp # Fragment reassembly
    │   ├── buffer/
    │   │   ├── FrameBuffer.hpp     # Producer-consumer buffer
    │   │   └── BufferObserver.hpp  # Concrete observers
    │   ├── state/
    │   │   ├── ClientState.hpp     # State pattern base
    │   │   └── ConcreteStates.hpp  # Init, Ready, Playing
    │   ├── ui/
    │   │   ├── ClientUI.hpp        # User interface
    │   │   └── FrameDisplay.hpp    # Video display
    │   └── utils/
    │       ├── Logger.hpp          # Singleton logger
    │       ├── Config.hpp          # Client config
    │       └── Metrics.hpp         # Performance metrics
    ├── src/
    │   ├── patterns/
    │   ├── network/
    │   ├── rtp/
    │   ├── buffer/
    │   ├── state/
    │   ├── ui/
    │   ├── utils/
    │   └── main.cpp                # Client entry point
    ├── config/
    │   └── client.conf
    └── tests/
        ├── test_rtsp.cpp
        ├── test_reassembly.cpp
        └── test_buffer.cpp
```
# Architecture
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

# Communication Flow
```bash
┌─────────────────────────────────────────────────────────────┐
│                    OVERALL SYSTEM FLOW                       │
└─────────────────────────────────────────────────────────────┘

CLIENT                                              SERVER
  │                                                    │
  │ ①──────── RTSP SETUP (TCP) ──────────────────────>│
  │                                                    │ Create session
  │                                                    │ Prepare video
  │ <──────── 200 OK + Session ID ──────────────────②│
  │                                                    │
  │ ③──────── RTSP PLAY (TCP) ───────────────────────>│
  │                                                    │ Start streaming
  │ <──────── 200 OK ────────────────────────────────④│
  │                                                    │
  │ <════════ RTP Packets (UDP) ═════════════════════⑤│ (loop)
  │ <════════ RTP Packets (UDP) ═════════════════════⑤│
  │ <════════ RTP Packets (UDP) ═════════════════════⑤│
  │                                                    │
  │ ⑥──────── RTSP PAUSE (TCP) ───────────────────────>│
  │                                                    │ Pause streaming
  │ <──────── 200 OK ────────────────────────────────⑦│
  │                                                    │
  │ ⑧──────── RTSP TEARDOWN (TCP) ────────────────────>│
  │                                                    │ Close session
  │ <──────── 200 OK ────────────────────────────────⑨│
  │                                                    │
```

# Component
```bash
┌──────────────────────────────────────────────────────────────┐
│                      CLIENT ARCHITECTURE                     │
└──────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                         User Interface                      │
│                  (Buttons: SETUP/PLAY/PAUSE/TEARDOWN)       │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────────┐
│                      RTSPClient                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         State Pattern (State Machine)                │  │
│  │  ┌──────┐   ┌──────┐   ┌────────┐   ┌─────────┐      │  │
│  │  │ INIT │──>│READY │──>│PLAYING │──>│ PAUSED  │      │  │
│  │  └──────┘   └──────┘   └────────┘   └─────────┘      │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                            │
│  RTSP Socket (TCP) ←→ Server                               │
└────────────────────────┬───────────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────────┐
│                      RTP Receiver                          │
│  RTP Socket (UDP) ←════ Server                             │
│  │                                                         │
│  ├──> Packet Parser                                        │
│  ├──> Fragment Reassembler (for HD)                        │
│  └──> Frame Buffer (Observer Pattern)                      │
└────────────────────────┬───────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   Frame Display Thread                      │
│  Pop frames from buffer → Display at 25 fps                 │
└─────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────┐
│                      SERVER ARCHITECTURE                     │
└──────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│                      RTSPServer                            │
│  Listen Socket (TCP) ←→ Client                             │
│  │                                                         │
│  └──> ServerWorker Thread (per client)                     │
│        ├──> Parse RTSP commands                            │
│        ├──> Manage session                                 │
│        └──> Control streaming                              │
└────────────────────────┬───────────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────────┐
│                      VideoStream                           │
│  Read MJPEG file → Extract frames                          │
└────────────────────────┬───────────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────────┐
│                   Encoding Strategy                        │
│  ┌──────────────────┐      ┌──────────────────┐            │
│  │  SD Strategy     │      │  HD Strategy     │            │
│  │  (Single packet) │      │  (Fragmentation) │            │
│  └──────────────────┘      └──────────────────┘            │
└────────────────────────┬───────────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────────┐
│                   RTPPacketBuilder                         │
│  Build RTP packets with proper headers                     │
└────────────────────────┬───────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   RTP Socket (UDP)                          │
│  Send packets to client at ~25 fps (40ms interval)          │
└─────────────────────────────────────────────────────────────┘
```

## 2. RTSP PROTOCOL LOGIC

### 2.1 RTSP Message Format
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