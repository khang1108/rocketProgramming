#ifndef COMMON_NETWORK_SOCKET_HPP
#define COMMON_NETWORK_SOCKET_HPP

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <cstdint>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

/**
 * @file Socket.hpp
 * @brief Cross-platform socket wrapper for TCP and UDP communication
 *
 * @details
 * Provides a unified interface for network programming across Windows and Linux/Unix.
 * Handles platform-specific differences in socket APIs and error codes.
 *
 * Key Features:
 * - RAII pattern: automatic resource management
 * - Exception-based error handling
 * - Support for both TCP (SOCK_STREAM) and UDP (SOCK_DGRAM)
 * - Thread-safe design (each socket instance is independent)
 * - Timeout support for blocking operations
 * - Cross-platform compatibility (Windows WinSock2 / POSIX sockets)
 *
 * Usage Patterns:
 *
 * **TCP Server:**
 * @code
 * Socket server(SocketType::TCP);
 * server.bind("0.0.0.0", 8554);
 * server.listen(5);
 * std::unique_ptr<Socket> client = server.accept();
 * client->send(data, length);
 * @endcode
 *
 * **TCP Client:**
 * @code
 * Socket client(SocketType::TCP);
 * client.connect("127.0.0.1", 8554);
 * client.send(request, requestSize);
 * int bytesReceived = client.receive(buffer, bufferSize);
 * @endcode
 *
 * **UDP Socket:**
 * @code
 * Socket udpSocket(SocketType::UDP);
 * udpSocket.bind("0.0.0.0", 25000);
 * udpSocket.setTimeout(500); // 0.5 second timeout
 *
 * // Receive
 * std::string fromAddr;
 * int fromPort;
 * int bytes = udpSocket.receiveFrom(buffer, bufferSize, fromAddr, fromPort);
 *
 * // Send
 * udpSocket.sendTo(data, length, "192.168.1.100", 25000);
 * @endcode
 *
 * @note Platform Differences:
 * - Windows: Uses WinSock2 API (WSAStartup required, error codes via WSAGetLastError)
 * - Linux/Unix: Uses POSIX sockets (direct system calls, error codes via errno)
 *
 * @note Thread Safety:
 * - Each Socket instance is independent (can be used in separate threads)
 * - Multiple threads can share a Socket if externally synchronized
 * - No global state (except Windows WSAStartup - handled internally)
 */

/**
 * @enum SocketType
 * @brief Defines the type of socket protocol
 */
enum class SocketType {
    TCP,  ///< TCP socket (SOCK_STREAM, reliable, connection-oriented)
    UDP   ///< UDP socket (SOCK_DGRAM, unreliable, connectionless)
};

/**
 * @class SocketException
 * @brief Base exception class for socket errors
 *
 * @details
 * Thrown when socket operations fail. Contains descriptive error message
 * combining the operation that failed and the system error code/message.
 *
 * @example
 * @code
 * try {
 *     socket.connect("invalid-host", 8080);
 * } catch (const SocketException& e) {
 *     std::cerr << "Connection failed: " << e.what() << std::endl;
 * }
 * @endcode
 */
class SocketException : public std::runtime_error {
  public:
    explicit SocketException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @class SocketTimeout
 * @brief Exception thrown when socket operation times out
 *
 * @details
 * Derived from SocketException. Thrown when a blocking operation
 * exceeds the timeout set via setTimeout().
 *
 * Common scenarios:
 * - receive() waits longer than timeout
 * - receiveFrom() waits longer than timeout
 * - accept() waits longer than timeout (if timeout is set)
 */
class SocketTimeout : public SocketException {
  public:
    explicit SocketTimeout(const std::string& message) : SocketException(message) {}
};

/**
 * @class Socket
 * @brief Cross-platform socket wrapper with RAII semantics
 *
 * @details
 * Encapsulates platform-specific socket operations and provides
 * a clean, exception-safe interface for network communication.
 *
 * **Design Principles:**
 * - RAII: Socket is automatically closed in destructor
 * - Move semantics: Sockets can be moved but not copied (unique ownership)
 * - Exception safety: All errors throw exceptions with descriptive messages
 * - Type safety: Separate methods for TCP and UDP operations
 *
 * **Internal State:**
 * - `sockfd`: Native socket file descriptor/handle
 * - `type`: TCP or UDP
 * - `connected`: Whether TCP socket is connected
 * - `bound`: Whether socket is bound to a local address
 * - `peerAddr`: Information about connected peer (TCP) or last sender (UDP)
 *
 * **Lifecycle:**
 * 1. Constructor: Creates socket (calls socket() system call)
 * 2. Configuration: bind(), listen(), connect(), setTimeout(), etc.
 * 3. Data transfer: send(), receive(), sendTo(), receiveFrom()
 * 4. Destructor: Closes socket (calls close()/closesocket())
 *
 * @note Non-copyable: Prevents accidental socket descriptor duplication
 * @note Movable: Allows transfer of ownership (e.g., returning from accept())
 */
class Socket {
  private:
    // ==================== MEMBER VARIABLES ====================

    SOCKET sockfd_;    ///< Native socket descriptor (int on Unix, SOCKET on Windows)
    SocketType type_;  ///< Socket type (TCP or UDP)
    bool connected_;   ///< True if TCP socket is connected
    bool bound_;       ///< True if socket is bound to local address

    struct sockaddr_in peerAddr_;  ///< Peer address information (connected TCP or last UDP sender)
    bool hasPeerInfo_;             ///< True if peerAddr_ contains valid information

    static bool wsaInitialized_;  ///< Windows only: tracks WSAStartup status

  public:
    // ==================== CONSTRUCTORS & DESTRUCTORS ====================

    /**
     * @brief Construct a socket of specified type
     *
     * @param socketType TCP or UDP
     * @throws SocketException if socket creation fails
     *
     * @details
     * Creates the underlying socket using system calls:
     * - TCP: socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
     * - UDP: socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
     *
     * On Windows, performs WSAStartup() on first socket creation.
     *
     * @note Does NOT bind or connect - call bind()/connect() separately
     *
     * @example
     * @code
     * Socket tcpSocket(SocketType::TCP);
     * Socket udpSocket(SocketType::UDP);
     * @endcode
     */
    explicit Socket(SocketType socketType);

    /**
     * @brief Destructor - automatically closes socket
     *
     * @details
     * Closes the socket if still open. Safe to call even if socket
     * was already closed manually. On Windows, calls WSACleanup()
     * when last socket is destroyed.
     *
     * @note No-throw guarantee: exceptions are caught and logged
     */
    ~Socket();

    // ==================== DELETED COPY OPERATIONS ====================

    /**
     * @brief Copy constructor - DELETED
     * @details Sockets cannot be copied (would duplicate file descriptor)
     */
    Socket(const Socket&) = delete;

    /**
     * @brief Copy assignment - DELETED
     * @details Sockets cannot be copied (would duplicate file descriptor)
     */
    Socket& operator=(const Socket&) = delete;

    // ==================== MOVE OPERATIONS ====================

    /**
     * @brief Move constructor - transfers ownership
     *
     * @param other Socket to move from (will be left in valid but empty state)
     *
     * @details
     * Transfers socket ownership. The source socket is left with
     * sockfd_ = INVALID_SOCKET and cannot be used after move.
     *
     * @example
     * @code
     * std::unique_ptr<Socket> client = server.accept();
     * Socket movedSocket = std::move(*client);
     * @endcode
     */
    Socket(Socket&& other) noexcept;

    /**
     * @brief Move assignment - transfers ownership
     *
     * @param other Socket to move from
     * @return Reference to this socket
     *
     * @details
     * Closes current socket (if open) and transfers ownership from other.
     */
    Socket& operator=(Socket&& other) noexcept;

    // ==================== TCP SERVER OPERATIONS ====================

    /**
     * @brief Bind socket to local address and port
     *
     * @param address Local IP address ("0.0.0.0" for all interfaces, "127.0.0.1" for localhost)
     * @param port Local port number (must be 0-65535, ports < 1024 require root on Linux)
     * @throws SocketException if bind fails
     * @throws SocketException if invalid port
     *
     * @details
     * Binds socket to specified local address/port. Required before:
     * - listen() for TCP server
     * - receiveFrom() for UDP socket
     *
     * Common bind errors:
     * - Port already in use (EADDRINUSE) - another process is using the port
     * - Permission denied (EACCES) - port < 1024 requires root
     * - Invalid address (EADDRNOTAVAIL) - address not available on this machine
     *
     * @note Use address="0.0.0.0" to listen on all network interfaces
     * @note Use port=0 to let OS assign an available port (call getLocalPort() after)
     *
     * @example
     * @code
     * Socket server(SocketType::TCP);
     * server.bind("0.0.0.0", 8554);  // Listen on all interfaces, port 8554
     * @endcode
     */
    void bind(const std::string& address, int port);

    /**
     * @brief Start listening for incoming connections (TCP only)
     *
     * @param backlog Maximum number of pending connections in queue (typically 5-128)
     * @throws SocketException if listen fails or called on UDP socket
     *
     * @details
     * Marks socket as passive (ready to accept connections). Must call bind() first.
     *
     * The backlog parameter determines how many clients can wait in queue
     * while server is busy handling another accept(). When queue is full,
     * new connection attempts are rejected.
     *
     * Typical backlog values:
     * - Small servers: 5-10
     * - Production servers: SOMAXCONN (usually 128 or more)
     *
     * @note Must call bind() before listen()
     * @note Only for TCP sockets (UDP is connectionless)
     * @note Does NOT block - just prepares socket to accept connections
     *
     * @example
     * @code
     * Socket server(SocketType::TCP);
     * server.bind("0.0.0.0", 8554);
     * server.listen(10);  // Allow up to 10 pending connections
     * @endcode
     */
    void listen(int backlog);

    /**
     * @brief Accept incoming TCP connection (blocking)
     *
     * @return Unique pointer to new Socket representing the client connection
     * @throws SocketException if accept fails
     * @throws SocketTimeout if timeout is set and expires
     *
     * @details
     * Blocks until a client connects. Returns a NEW socket that represents
     * the connection to that specific client. The original server socket
     * remains open to accept more connections.
     *
     * Flow:
     * 1. Client calls connect() → TCP handshake
     * 2. Connection enters server's accept queue
     * 3. accept() removes connection from queue and returns new socket
     * 4. Server can now communicate with client via returned socket
     * 5. Original server socket continues to accept() new clients
     *
     * @note Blocks until connection arrives (or timeout if set)
     * @note Must call bind() and listen() before accept()
     * @note Returned socket is connected and ready for send()/receive()
     *
     * @example
     * @code
     * Socket server(SocketType::TCP);
     * server.bind("0.0.0.0", 8554);
     * server.listen(5);
     *
     * while (true) {
     *     std::unique_ptr<Socket> client = server.accept();
     *     std::thread([client = std::move(client)]() {
     *         // Handle client in separate thread
     *         uint8_t buffer[256];
     *         int bytes = client->receive(buffer, sizeof(buffer));
     *         client->send(buffer, bytes);  // Echo back
     *     }).detach();
     * }
     * @endcode
     */
    std::unique_ptr<Socket> accept();

    // ==================== TCP CLIENT OPERATIONS ====================

    /**
     * @brief Connect to remote TCP server (blocking)
     *
     * @param host Hostname or IP address (e.g., "127.0.0.1", "example.com")
     * @param port Remote port number
     * @throws SocketException if connection fails
     * @throws SocketTimeout if connection timeout expires
     *
     * @details
     * Establishes TCP connection to remote server. Performs 3-way handshake:
     * 1. Client → SYN → Server
     * 2. Server → SYN-ACK → Client
     * 3. Client → ACK → Server
     *
     * Hostname resolution:
     * - Supports both IPv4 addresses ("192.168.1.1") and hostnames ("example.com")
     * - Uses getaddrinfo() for DNS lookup
     * - Tries first available address if multiple IPs returned
     *
     * Common connection errors:
     * - ECONNREFUSED: Server not listening on that port
     * - ETIMEDOUT: Network unreachable or server not responding
     * - EHOSTUNREACH: No route to host
     *
     * @note Blocks until connection succeeds or fails
     * @note After successful connect(), socket is ready for send()/receive()
     * @note Only for TCP sockets (UDP is connectionless)
     *
     * @example
     * @code
     * Socket client(SocketType::TCP);
     * client.connect("127.0.0.1", 8554);
     *
     * std::string request = "SETUP movie.Mjpeg RTSP/1.0\r\n";
     * client.send(request.c_str(), request.size());
     * @endcode
     */
    void connect(const std::string& host, int port);

    // ==================== TCP DATA TRANSFER ====================

    /**
     * @brief Send data over TCP connection
     *
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     * @return Number of bytes actually sent (may be less than length)
     * @throws SocketException if send fails or socket not connected
     *
     * @details
     * Sends data over established TCP connection. May not send all data
     * in one call (especially for large buffers). Caller should check
     * return value and call send() again for remaining data if needed.
     *
     * TCP guarantees:
     * - Data arrives in order
     * - No duplicates
     * - Error detection via checksum
     *
     * @note Must call connect() (client) or receive from accept() (server) first
     * @note May send fewer bytes than requested (check return value)
     * @note Only for TCP sockets
     *
     * @example
     * @code
     * Socket client(SocketType::TCP);
     * client.connect("127.0.0.1", 8554);
     *
     * std::string message = "Hello Server!";
     * int sent = 0;
     * while (sent < message.size()) {
     *     int bytes = client.send(message.c_str() + sent, message.size() - sent);
     *     sent += bytes;
     * }
     * @endcode
     */
    int send(const uint8_t* data, size_t length);

    /**
     * @brief Receive data from TCP connection (blocking)
     *
     * @param buffer Pointer to buffer to store received data
     * @param bufferSize Maximum number of bytes to receive
     * @return Number of bytes received (0 = connection closed, -1 = error)
     * @throws SocketException if receive fails
     * @throws SocketTimeout if timeout expires
     *
     * @details
     * Receives data from TCP connection. Blocks until:
     * - At least 1 byte arrives
     * - Connection is closed by peer (returns 0)
     * - Timeout expires (if setTimeout() was called)
     * - Error occurs
     *
     * TCP stream semantics:
     * - May return fewer bytes than bufferSize (even if more data is available)
     * - No message boundaries (must implement your own framing)
     * - Returns 0 when peer closes connection gracefully
     *
     * @note Must call connect() or receive from accept() first
     * @note Returns 0 when connection closed (not an error)
     * @note May return fewer bytes than requested (this is normal)
     * @note Only for TCP sockets
     *
     * @example
     * @code
     * Socket client(SocketType::TCP);
     * client.connect("127.0.0.1", 8554);
     * client.setTimeout(5);  // 5 second timeout
     *
     * uint8_t buffer[1024];
     * int bytesReceived = client.receive(buffer, sizeof(buffer));
     *
     * if (bytesReceived == 0) {
     *     std::cout << "Connection closed" << std::endl;
     * } else {
     *     std::string message(buffer, buffer + bytesReceived);
     *     std::cout << "Received: " << message << std::endl;
     * }
     * @endcode
     */
    int receive(uint8_t* buffer, size_t bufferSize);

    // ==================== UDP OPERATIONS ====================

    /**
     * @brief Send UDP datagram to specific address
     *
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     * @param destAddress Destination IP address (e.g., "192.168.1.100")
     * @param destPort Destination port number
     * @return Number of bytes sent
     * @throws SocketException if send fails
     *
     * @details
     * Sends UDP datagram to specified destination. Does NOT require connect().
     * Each datagram is independent and self-contained.
     *
     * UDP characteristics:
     * - Unreliable: packets may be lost, duplicated, or arrive out of order
     * - No connection: each sendTo() is independent
     * - Fast: no handshake overhead
     * - Message boundaries preserved (unlike TCP)
     *
     * Maximum safe datagram size:
     * - Ethernet MTU: 1500 bytes
     * - IP header: 20 bytes
     * - UDP header: 8 bytes
     * - Safe payload: ~1472 bytes
     * - Recommended: 1400 bytes to avoid fragmentation
     *
     * @note Only for UDP sockets
     * @note Does NOT require bind() or connect()
     * @note Entire datagram sent atomically (all or nothing)
     * @note May be lost - no retransmission
     *
     * @example
     * @code
     * Socket udpSocket(SocketType::UDP);
     *
     * // RTP packet sending
     * std::vector<uint8_t> rtpPacket = buildRTPPacket(frameData);
     * udpSocket.sendTo(rtpPacket.data(), rtpPacket.size(),
     *                  "192.168.1.100", 25000);
     * @endcode
     */
    int sendTo(const uint8_t* data, size_t length, const std::string& destAddress, int destPort);

    /**
     * @brief Receive UDP datagram (blocking)
     *
     * @param buffer Pointer to buffer to store received data
     * @param bufferSize Maximum number of bytes to receive
     * @param sourceAddress [out] Source IP address (filled by function)
     * @param sourcePort [out] Source port number (filled by function)
     * @return Number of bytes received
     * @throws SocketException if receive fails
     * @throws SocketTimeout if timeout expires
     *
     * @details
     * Receives one complete UDP datagram. Blocks until:
     * - A datagram arrives
     * - Timeout expires (if setTimeout() was called)
     * - Error occurs
     *
     * Source information:
     * - sourceAddress and sourcePort are filled with sender's information
     * - Useful for replying back to sender
     * - Can use for filtering/validation
     *
     * Buffer size considerations:
     * - If buffer too small, datagram is truncated (data lost!)
     * - Recommended: 2048 bytes for RTP packets
     * - UDP preserves message boundaries (receives complete datagram)
     *
     * @note Must call bind() before receiveFrom()
     * @note Only for UDP sockets
     * @note Returns complete datagram or nothing (no partial receives)
     * @note Check bufferSize >= expected packet size to avoid truncation
     *
     * @example
     * @code
     * Socket udpSocket(SocketType::UDP);
     * udpSocket.bind("0.0.0.0", 25000);
     * udpSocket.setTimeout(500);  // 0.5 second timeout
     *
     * uint8_t buffer[2048];
     * std::string fromAddr;
     * int fromPort;
     *
     * while (running) {
     *     try {
     *         int bytes = udpSocket.receiveFrom(buffer, sizeof(buffer),
     *                                           fromAddr, fromPort);
     *         RTPPacket packet(buffer, bytes);
     *         processPacket(packet);
     *     } catch (const SocketTimeout& e) {
     *         // Timeout - check stop flag and continue
     *         if (shouldStop) break;
     *     }
     * }
     * @endcode
     */
    int receiveFrom(uint8_t* buffer, size_t bufferSize, std::string& sourceAddress,
                    int& sourcePort);

    // ==================== SOCKET OPTIONS ====================

    /**
     * @brief Set timeout for blocking operations
     *
     * @param milliseconds Timeout in milliseconds (0 = infinite, no timeout)
     * @throws SocketException if setsockopt fails
     *
     * @details
     * Sets timeout for blocking socket operations (receive, receiveFrom, accept).
     * When timeout expires, operation throws SocketTimeout exception.
     *
     * Typical usage:
     * - RTP receiver: 500ms timeout (check stop flag periodically)
     * - RTSP client: 5000ms timeout (5 seconds for server response)
     * - Production: 30000ms (30 seconds)
     *
     * @note Timeout=0 means infinite (blocks forever until data arrives)
     * @note Affects: receive(), receiveFrom(), accept()
     * @note Does NOT affect: send(), sendTo(), connect()
     *
     * @example
     * @code
     * Socket udpSocket(SocketType::UDP);
     * udpSocket.bind("0.0.0.0", 25000);
     * udpSocket.setTimeout(500);  // 0.5 second
     *
     * try {
     *     int bytes = udpSocket.receiveFrom(buffer, size, addr, port);
     * } catch (const SocketTimeout& e) {
     *     // Timeout - no data received within 500ms
     * }
     * @endcode
     */
    void setTimeout(int milliseconds);

    /**
     * @brief Enable/disable address reuse (SO_REUSEADDR)
     *
     * @param reuse true to enable, false to disable
     * @throws SocketException if setsockopt fails
     *
     * @details
     * Allows immediate rebind to address after previous socket closed.
     * Without this, bind() fails with "Address already in use" for ~2 minutes
     * after server restart (TIME_WAIT state).
     *
     * Use cases:
     * - Server restart: immediately reuse same port
     * - Development: avoid waiting for TIME_WAIT
     * - Multiple servers on same port (with different IPs)
     *
     * @note Should be called BEFORE bind()
     * @note Essential for server development (avoid "Address already in use")
     * @note Safe to use (does not break TCP protocol)
     *
     * @example
     * @code
     * Socket server(SocketType::TCP);
     * server.setReuseAddress(true);  // Must call before bind()
     * server.bind("0.0.0.0", 8554);
     * server.listen(5);
     * @endcode
     */
    void setReuseAddress(bool reuse);

    /**
     * @brief Set socket to non-blocking mode
     *
     * @param nonBlocking true for non-blocking, false for blocking
     * @throws SocketException if fcntl/ioctlsocket fails
     *
     * @details
     * Non-blocking mode: operations return immediately instead of blocking.
     *
     * Behavior changes:
     * - accept(): Returns immediately (error if no connection pending)
     * - receive(): Returns immediately (error if no data available)
     * - send(): Returns immediately (may send partial data)
     * - connect(): Returns immediately (connection in progress)
     *
     * Error codes for "would block":
     * - Linux: EAGAIN or EWOULDBLOCK
     * - Windows: WSAEWOULDBLOCK
     *
     * @note Requires careful error handling (check for EAGAIN/EWOULDBLOCK)
     * @note Usually used with select()/poll()/epoll for event-driven I/O
     * @note Not recommended for beginners (blocking mode is simpler)
     *
     * @example
     * @code
     * Socket server(SocketType::TCP);
     * server.bind("0.0.0.0", 8554);
     * server.listen(5);
     * server.setNonBlocking(true);
     *
     * while (true) {
     *     try {
     *         auto client = server.accept();
     *         handleClient(std::move(client));
     *     } catch (const SocketException& e) {
     *         // No connection pending - do other work
     *         std::this_thread::sleep_for(std::chrono::milliseconds(100));
     *     }
     * }
     * @endcode
     */
    void setNonBlocking(bool nonBlocking);

    /**
     * @brief Set TCP No-Delay option (disable Nagle's algorithm)
     *
     * @param nodelay true to disable Nagle, false to enable
     * @throws SocketException if setsockopt fails
     *
     * @details
     * Nagle's algorithm: buffers small packets to reduce network overhead.
     *
     * With Nagle (default):
     * - Small packets are buffered and sent together
     * - Reduces packet count (good for throughput)
     * - Increases latency (bad for real-time)
     *
     * Without Nagle (TCP_NODELAY):
     * - Each send() creates a packet immediately
     * - Lower latency (good for real-time, gaming, RTSP)
     * - More packets (slightly higher overhead)
     *
     * @note Only for TCP sockets
     * @note Recommended for RTSP control channel (low latency needed)
     * @note Not needed for RTP data (uses UDP)
     *
     * @example
     * @code
     * Socket rtspSocket(SocketType::TCP);
     * rtspSocket.connect("127.0.0.1", 8554);
     * rtspSocket.setNoDelay(true);  // Immediate send for RTSP commands
     * @endcode
     */
    void setNoDelay(bool nodelay);

    /**
     * @brief Set socket send/receive buffer size
     *
     * @param sendBufferSize Send buffer size in bytes (0 = use default)
     * @param receiveBufferSize Receive buffer size in bytes (0 = use default)
     * @throws SocketException if setsockopt fails
     *
     * @details
     * Sets kernel buffer sizes for send and receive operations.
     *
     * Default sizes (typical):
     * - Linux: 87380 bytes (receive), 16384 bytes (send)
     * - Windows: 8192 bytes (both)
     *
     * When to increase:
     * - High-bandwidth streaming (HD video): 256KB-1MB
     * - High latency networks: larger buffers smooth out delays
     * - Bulk data transfer: larger buffers improve throughput
     *
     * When to decrease:
     * - Low-latency required: smaller buffers reduce queuing delay
     * - Memory constrained systems: many concurrent connections
     *
     * @note OS may adjust requested size to limits (check with getsockopt)
     * @note Larger is not always better (increases memory usage)
     * @note Must set before bind() for full effect
     *
     * @example
     * @code
     * Socket udpSocket(SocketType::UDP);
     * udpSocket.setBufferSize(0, 1048576);  // 1MB receive buffer for RTP
     * udpSocket.bind("0.0.0.0", 25000);
     * @endcode
     */
    void setBufferSize(int sendBufferSize, int receiveBufferSize);

    // ==================== SOCKET INFORMATION ====================

    /**
     * @brief Get local port number
     *
     * @return Port number socket is bound to (0 if not bound)
     * @throws SocketException if getsockname fails
     *
     * @details
     * Returns the local port number. Useful when bind(0) was used
     * to let OS assign an available port.
     *
     * @example
     * @code
     * Socket udpSocket(SocketType::UDP);
     * udpSocket.bind("0.0.0.0", 0);  // Let OS assign port
     * int assignedPort = udpSocket.getLocalPort();
     * std::cout << "Listening on port: " << assignedPort << std::endl;
     * @endcode
     */
    int getLocalPort() const;

    /**
     * @brief Get local IP address
     *
     * @return IP address as string (e.g., "192.168.1.100")
     * @throws SocketException if getsockname fails
     *
     * @details
     * Returns the local IP address socket is bound to.
     * Returns "0.0.0.0" if bound to all interfaces.
     */
    std::string getLocalAddress() const;

    /**
     * @brief Get peer IP address (TCP only)
     *
     * @return Peer IP address as string
     * @throws SocketException if not connected or getpeername fails
     *
     * @details
     * Returns IP address of connected peer (TCP client for server,
     * or server for client). Only valid after connect() or accept().
     *
     * @example
     * @code
     * auto client = server.accept();
     * std::string clientIP = client->getPeerAddress();
     * int clientPort = client->getPeerPort();
     * std::cout << "Client connected from: " << clientIP
     *           << ":" << clientPort << std::endl;
     * @endcode
     */
    std::string getPeerAddress() const;

    /**
     * @brief Get peer port number (TCP only)
     *
     * @return Peer port number
     * @throws SocketException if not connected or getpeername fails
     */
    int getPeerPort() const;

    /**
     * @brief Check if socket is connected
     *
     * @return true if TCP socket is connected, false otherwise
     *
     * @details
     * For TCP: returns true after successful connect() or accept()
     * For UDP: always returns false (UDP is connectionless)
     */
    bool isConnected() const { return connected_; }

    /**
     * @brief Check if socket is bound
     *
     * @return true if socket is bound to local address, false otherwise
     */
    bool isBound() const { return bound_; }

    /**
     * @brief Get socket type
     *
     * @return SocketType::TCP or SocketType::UDP
     */
    SocketType getType() const { return type_; }

    // ==================== SOCKET CONTROL ====================

    /**
     * @brief Close socket
     *
     * @details
     * Closes the socket and releases resources. Safe to call multiple times.
     * Automatically called by destructor.
     *
     * After close():
     * - Socket descriptor becomes invalid
     * - All operations will throw SocketException
     * - TCP connection is terminated (FIN sent)
     * - Socket can be reused with new socket() call
     *
     * @note No-throw guarantee: safe to call in destructor
     * @note Automatically called by destructor
     */
    void close();

    /**
     * @brief Shutdown socket (graceful close)
     *
     * @param how Shutdown mode: SHUT_RD (0), SHUT_WR (1), SHUT_RDWR (2)
     * @throws SocketException if shutdown fails
     *
     * @details
     * Gracefully closes one or both directions of TCP connection:
     * - SHUT_RD (0): No more receives (but can still send)
     * - SHUT_WR (1): No more sends (but can still receive) - sends FIN
     * - SHUT_RDWR (2): No more send or receive - full shutdown
     *
     * Difference from close():
     * - shutdown(): signals intent to close (sends FIN)
     * - close(): releases socket descriptor
     * - Typical: shutdown(SHUT_WR) then receive() until 0, then close()
     *
     * @note Only for TCP sockets
     * @note Recommended for graceful connection termination
     *
     * @example
     * @code
     * client.shutdown(1);  // SHUT_WR - no more sends
     *
     * // Drain remaining data
     * uint8_t buffer[1024];
     * while (client.receive(buffer, sizeof(buffer)) > 0) {}
     *
     * client.close();
     * @endcode
     */
    void shutdown(int how);

  private:
    // ==================== INTERNAL HELPER METHODS ====================

    /**
     * @brief Initialize Windows Sockets (Windows only)
     * @throws SocketException if WSAStartup fails
     */
    static void initializeWinSock();

    /**
     * @brief Cleanup Windows Sockets (Windows only)
     */
    static void cleanupWinSock();

    /**
     * @brief Get last error message
     * @return Descriptive error message from system
     */
    static std::string getLastErrorString();

    /**
     * @brief Check if error is EAGAIN/EWOULDBLOCK (non-blocking operation)
     * @return true if operation would block
     */
    static bool isWouldBlock();

    /**
     * @brief Private constructor for accept() (creates socket from existing descriptor)
     * @param sockfd Existing socket descriptor
     * @param type Socket type
     * @param peerAddr Peer address information
     */
    Socket(SOCKET sockfd, SocketType type, const sockaddr_in& peerAddr);
};

#endif