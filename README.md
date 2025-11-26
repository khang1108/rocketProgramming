# rocketProgramming
rocketProgramming is a Realtime Streaming Video application in which a server sends a video to a client in an end system. All of things in this project are built in C++ and Socket STL.

# Architecture
```bash
┌─────────────────────────────────────────────────────────────┐
│                    DEPLOYMENT DIAGRAM                        │
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
│  │ RTPPacketBuilder│ │            │  │ FrameReassembler│ │
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