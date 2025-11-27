#ifndef SOCKET_CLIENT_HPP
#define SOCKET_CLIENT_HPP

#include <string>
#include <memory>
#include <stdexcept>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

//@brief: To store the type of SOCKET TCP or UDP
enum class SocketType
{
    
};

//@brief: Customize exceptions for socket
class SocketException : public std::runtime_error
{
public:
    explicit SocketException(const std::string &message) : std::runtime_error(message){}
};
class SocketTimeout : public SocketException 
{
public:
    explicit SocketTimeout(const std::string &message) : SocketException(message){}
};

class Socket
{
private:
    SOCKET sockfd;
    SocketType type; //* TCP or UDP
    bool connected;
    bool bound;

    //* To store peer information
    struct sockaddr_in peerAddr;
    bool hasPeerInfo;
public:
    /*@brief: 
     * Create socket based on type
     * 
     * TCP: SOCK_STREAM, IPPROTO_TCP
     * UDP: SOCK_DGRAM, IPPROTO_UDP
     */
    Socket(SocketType socketType);
    ~Socket();

    //TODO: Disable copy constructor. Socket MUSTN'T be copied. 
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    //TODO: Move constructor and assignment
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // ==================== TCP Methods ====================
    
    /*@brief:
     * Connect to remote host (TCP client)
     * 
     * @param host Hostname or IP address
     * @param port Port number
     * @throws SocketException on failure
     */
    /*TODO
     * TCP client connect
     * 
     * Steps:
     * 1. Resolve hostname to IP
     * 2. Fill sockaddr_in structure
     * 3. Call connect()
     * 4. Check result
     */
    void connect(const std::string &host, int port);

    /*@brief
     * Bind socket to local port
     * 
     * @param port Local port number (0 = any available port)
     * @throws SocketException on failure
     */
    /*TODO
     * Bind socket to port
     * 
     * Used by:
     * - Server (TCP listen socket)
     * - Client (UDP RTP socket)
     */
    void bind(int port);

    /*TODO
     * Start listening (TCP server only)
     */
    void listen(int backup);

    /*TODO
     * Accept incoming connection
     * 
     * Returns: New socket for client
     */
    std::unique_ptr<Socket> accept();

    //! UDP specific
    /*TODO
     * UDP sendto
     * 
     * Send datagram to specific address
     */
    void sendTo(const void* data, size_t len, const std::string &host, int port);
    ssize_t receiveFrom(void *buffer, size_t len, std::string &fromHost, int &fromPort);

    //! Common parts
    /*TODO
     * Send data (TCP or connected UDP)
     * 
     * Returns: Bytes sent, or -1 on error
     */
    ssize_t send(const void *data, size_t len);

    /*TODO
     * Receive data
     * 
     * Blocking until:
     * - Data arrives
     * - Timeout (if set)
     * - Connection closed
     */
    ssize_t receive(void *buffer, size_t len);

    /*TODO
     * Set receive timeout
     * 
     * Important for RTP receiver:
     * - Timeout = 0.5s typical
     * - Allows checking stop flag periodically
     */
    void setTimeout(int seconds);
    void close();

    //! Info
    std::string getPeerAddress() const;
    int getPeerPort() const;
};
#endif