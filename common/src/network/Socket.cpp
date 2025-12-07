#include "network/Socket.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#endif

Socket::Socket(SocketType socketType) {
//* Initialize WinSock
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    int sockType = (socketType == SocketType::TCP) ? SOCK_STREAM : SOCK_DGRAM;
    int protocol = (socketType == SocketType::TCP) ? IPPROTO_TCP : IPPROTO_UDP;

    //* Create socket
    sockfd_ = socket(AF_INET, sockType, protocol);
    if (sockfd_ == INVALID_SOCKET) {
        throw SocketException("socket failed");
    }

    type_ = socketType;
    connected_ = false;
    bound_ = false;
    hasPeerInfo_ = false;

    memset(&peerAddr_, 0, sizeof(peerAddr_));
}
Socket::~Socket() {
    if (sockfd_ != INVALID_SOCKET) {
        close();
    }
#ifdef _WIN32
    WSACleanup();
#endif
}
Socket::Socket(Socket&& other) noexcept
    : sockfd_(other.sockfd_),
      type_(other.type_),
      connected_(other.connected_),
      bound_(other.bound_),
      peerAddr_(other.peerAddr_),
      hasPeerInfo_(other.hasPeerInfo_) {
    other.sockfd_ = INVALID_SOCKET;
    other.connected_ = false;
    other.bound_ = false;
    other.hasPeerInfo_ = false;
    memset(&other.peerAddr_, 0, sizeof(other.peerAddr_));
}
Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();

        sockfd_ = other.sockfd_;
        type_ = other.type_;
        connected_ = other.connected_;
        bound_ = other.bound_;
        hasPeerInfo_ = other.hasPeerInfo_;
        peerAddr_ = other.peerAddr_;

        other.sockfd_ = INVALID_SOCKET;
        other.connected_ = false;
        other.bound_ = false;
        other.hasPeerInfo_ = false;
        memset(&other.peerAddr_, 0, sizeof(other.peerAddr_));
    }
    return *this;
}
void Socket::bind(const std::string& address, int port) {
    if (port < 0 || port > 65535) {
        throw SocketException("invalid port");
    }

    if (bound_) {
        throw SocketException("socket already bound");
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if (::bind(sockfd_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        throw SocketException("bind failed");
    }
    bound_ = true;
}
void Socket::listen(int backlog) {
    if (type_ != SocketType::TCP) {
        throw SocketException("listen called on UDP socket");
    }
    if (!bound_) {
        throw SocketException("socket not bound");
    }

    if (backlog <= 0) {
        throw SocketException("invalid backlog");
    }

    if (::listen(sockfd_, backlog) == SOCKET_ERROR) {
        throw SocketException("listen failed");
    }
}
std::unique_ptr<Socket> Socket::accept() {
    if (type_ != SocketType::TCP) {
        throw SocketException("accept called on UDP socket");
    }
    if (!bound_) {
        throw SocketException("socket not bound");
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrLen = sizeof(addr);

    // 3. Gọi system call accept() (BLOCKING - chờ đến khi có client)
    SOCKET clientSocket = ::accept(sockfd_, (sockaddr*)&addr, &addrLen);

    if (clientSocket == INVALID_SOCKET) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            throw SocketTimeout("accept timed out");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            throw SocketTimeout("accept timed out");
        }
#endif
        throw SocketException("accept failed");
    }

    return std::unique_ptr<Socket>(new Socket(clientSocket, type_, addr));
}
void Socket::connect(const std::string& host, int port) {
    if (type_ != SocketType::TCP) {
        throw SocketException("connect called on UDP socket");
    }
    if (connected_) {
        throw SocketException("socket already connected");
    }
    if (port < 0 || port > 65535) {
        throw SocketException("invalid port");
    }

    struct addrinfo hints, *result = nullptr, *rp = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr = std::to_string(port);
    int ret = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (ret != 0) {
#ifdef _WIN32
        throw SocketException("DNS lookup failed for host: " + host +
                              "(error: " + std::to_string(ret) + ")");
#else
        throw SocketException("DNS lookup failed for host: " + host + "(" +
                              std::string(gai_strerror(ret)) + ")");
#endif
    }

    bool connected = false;
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        //* Try to connect to the address
        if (::connect(sockfd_, rp->ai_addr, rp->ai_addrlen) == 0) {
            connected = true;

            memcpy(&peerAddr_, rp->ai_addr, rp->ai_addrlen);
            hasPeerInfo_ = true;
            break;
        }
    }

    freeaddrinfo(result);

    if (!connected) {
#ifdef _WIN32
        int error = WSAGetLastError();
        std::string errorMsg;

        switch (error) {
            case WSAECONNREFUSED:
                errorMsg = "Connection refused (server not listening)";
                break;
            case WSAETIMEDOUT:
                errorMsg = "Connection timed out";
                throw SocketTimeout("connect() timeout to " + host + ":" + std::to_string(port));
                break;
            case WSAEHOSTUNREACH:
                errorMsg = "No route to host";
                break;
            default:
                errorMsg = "Connect failed (error: " + std::to_string(error) + ")";
                break;
        }
        throw SocketException(errorMsg);
#else
        std::string errorMsg;
        switch (errno) {
            case ECONNREFUSED:
                errorMsg = "Connection refused (server not listening)";
                break;
            case ETIMEDOUT:
                errorMsg = "Connection timed out";
                throw SocketTimeout("connect() timeout to " + host + ":" + std::to_string(port));
            case EHOSTUNREACH:
                errorMsg = "No route to host";
                break;
            default:
                errorMsg = "Connect failed: ";
                break;
        }
        throw SocketException(errorMsg);
#endif
    }
    connected_ = true;
}
int Socket::send(const uint8_t* data, size_t length) {
    if (type_ != SocketType::TCP) {
        throw SocketException("send called on UDP socket");
    }
    if (!connected_) {
        throw SocketException("socket not connected");
    }

    if (length <= 0 || data == nullptr) {
        throw SocketException("invalid length");
    }

    size_t totalSent = 0;
    while (totalSent < length) {
        int bytes = ::send(sockfd_, (const char*)(data + totalSent), length - totalSent, 0);

        if (bytes == SOCKET_ERROR) {
        #ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                throw SocketTimeout("send() timed out");
            }
            throw SocketException("send() failed: " + std::to_string(error));
        #else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                throw SocketTimeout("send() timed out");
            }
            throw SocketException("send() failed: " + std::string(strerror(errno)));
        #endif
        }

        if (bytes == 0) {
            throw SocketException("send() failed: connection closed by peer");
        }

        totalSent += bytes;
    }

    return totalSent;
}
int Socket::receive(uint8_t* buffer, size_t bufferSize) {
    if (type_ != SocketType::TCP) {
        throw SocketException("receive called on UDP socket");
    }
    if (!connected_) {
        throw SocketException("socket not connected");
    }

    if (bufferSize <= 0 || buffer == nullptr) {
        throw SocketException("invalid buffer size");
    }

    int bytesReceived = ::recv(sockfd_, (char*)buffer, bufferSize, 0);

    if (bytesReceived == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
            throw SocketTimeout("receive() timed out");
        }
        throw SocketException("receive() failed: " + std::to_string(error));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            throw SocketTimeout("receive() timed out");
        }
        throw SocketException("receive() failed: " + std::string(strerror(errno)));
#endif
    }

    return bytesReceived;
}
int Socket::sendTo(const uint8_t* data, size_t length, const std::string& destAddress,
                   int destPort) {
    if (type_ != SocketType::UDP) {
        throw SocketException("sendTo called on TCP socket");
    }
    if (length <= 0 || data == nullptr) {
        throw SocketException("invalid length");
    }

    if (destPort < 0 || destPort > 65535) {
        throw SocketException("invalid destination port");
    }

    sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destPort);

    //* Convert destination address from string to binary
    if (inet_pton(AF_INET, destAddress.c_str(), &destAddr.sin_addr) == 0) {
        throw SocketException("invalid destination address");
    }

    int bytesSent = ::sendto(sockfd_, (const char*)data, length, 0, (struct sockaddr*)&destAddr,
                             sizeof(destAddr));

    if (bytesSent == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        throw SocketException("sendTo() failed: " + std::to_string(error));
#else
        throw SocketException("sendTo() failed: " + std::string(strerror(errno)));
#endif
    }
    return bytesSent;
}
int Socket::receiveFrom(uint8_t* buffer, size_t bufferSize, std::string& sourceAddress,
                        int& sourcePort) {
    if (type_ != SocketType::UDP) {
        throw SocketException("receiveFrom called on TCP socket");
    }
    if (!bound_) {
        throw SocketException("socket not bound");
    }
    if (bufferSize <= 0 || buffer == nullptr) {
        throw SocketException("invalid buffer size");
    }

    sockaddr_in sourceAddr;
    memset(&sourceAddr, 0, sizeof(sourceAddr));
    socklen_t sourceAddrLen = sizeof(sourceAddr);

    int bytesReceived = ::recvfrom(sockfd_, (char*)buffer, bufferSize, 0,
                                   (struct sockaddr*)&sourceAddr, &sourceAddrLen);

    if (bytesReceived == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
            throw SocketTimeout("receiveFrom() timed out");
        }
        throw SocketException("receiveFrom() failed: " + std::to_string(error));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            throw SocketTimeout("receiveFrom() timed out");
        }
        throw SocketException("receiveFrom() failed: " + std::string(strerror(errno)));
#endif
    }

    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &sourceAddr.sin_addr, ipStr, sizeof(ipStr)) == nullptr) {
        throw SocketException("receiveFrom() failed: " + std::string(strerror(errno)));
    }
    sourceAddress = std::string(ipStr);
    sourcePort = ntohs(sourceAddr.sin_port);

    return bytesReceived;
}
void Socket::setTimeout(int milliseconds) {
    if (milliseconds < 0) {
        throw SocketException("invalid timeout");
    }

#ifdef _WIN32
    DWORD timeout = milliseconds;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) ==
        SOCKET_ERROR) {
        int error = WSAGetLastError();
        throw SocketException("setTimeout() failed: " + std::to_string(error));
    }
#else
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) ==
        SOCKET_ERROR) {
        throw SocketException("setTimeout() failed: " + std::string(strerror(errno)));
    }
#endif
}
void Socket::setReuseAddress(bool reuse) {
    int optVal = reuse ? 1 : 0;
    if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, sizeof(optVal)) ==
        SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        throw SocketException("setReuseAddress() failed: " + std::to_string(error));
#else
        throw SocketException("setReuseAddress() failed: " + std::string(strerror(errno)));
#endif
    }
}
void Socket::setNonBlocking(bool nonBlocking) {
#ifdef _WIN32
    u_long mode = nonBlocking ? 1 : 0;
    if (ioctlsocket(sockfd_, FIONBIO, &mode) == SOCKET_ERROR) {
        throw SocketException("ioctlsocket(FIONBIO) failed: " + std::to_string(WSAGetLastError()));
    }
#else
    int flags = fcntl(sockfd_, F_GETFL, 0);
    if (flags == -1) {
        throw SocketException("fcntl(F_GETFL) failed: " + std::string(strerror(errno)));
    }

    if (nonBlocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(sockfd_, F_SETFL, flags) == -1) {
        throw SocketException("fcntl(F_SETFL) failed: " + std::string(strerror(errno)));
    }
#endif
}
void Socket::setNoDelay(bool nodelay) {
    if (type_ != SocketType::TCP) {
        throw SocketException("setNoDelay called on UDP socket");
    }

    int optVal = nodelay ? 1 : 0;
    if (::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, sizeof(optVal)) ==
        SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        throw SocketException("setNoDelay() failed: " + std::to_string(error));
#else
        throw SocketException("setNoDelay() failed: " + std::string(strerror(errno)));
#endif
    }
}
void Socket::setBufferSize(int sendBufferSize, int receiveBufferSize) {
    if (sendBufferSize > 0) {
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize,
                         sizeof(sendBufferSize)) == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            throw SocketException("setBufferSize(send) failed: " + std::to_string(error));
#else
            throw SocketException("setBufferSize(send) failed: " + std::string(strerror(errno)));
#endif
        }
    }
    if (receiveBufferSize > 0) {
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_RCVBUF, (const char*)&receiveBufferSize,
                         sizeof(receiveBufferSize)) == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            throw SocketException("setBufferSize(receive) failed: " + std::to_string(error));
#else
            throw SocketException("setBufferSize(receive) failed: " + std::string(strerror(errno)));
#endif
        }
    }
}
int Socket::getLocalPort() const {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrLen = sizeof(addr);

    if (getsockname(sockfd_, (sockaddr*)&addr, &addrLen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        throw SocketException("getLocalPort() failed: " + std::to_string(error));
#else
        throw SocketException("getLocalPort() failed: " + std::string(strerror(errno)));
#endif
    }
    return ntohs(addr.sin_port);
}
std::string Socket::getLocalAddress() const {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrLen = sizeof(addr);

    if (getsockname(sockfd_, (sockaddr*)&addr, &addrLen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        throw SocketException("getLocalAddress() failed: " + std::to_string(error));
#else
        throw SocketException("getLocalAddress() failed: " + std::string(strerror(errno)));
#endif
    }

    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr)) == nullptr) {
        throw SocketException("getLocalAddress() failed: " + std::string(strerror(errno)));
    }
    return std::string(ipStr);
}
std::string Socket::getPeerAddress() const {
    if (type_ != SocketType::TCP) {
        throw SocketException("getPeerAddress called on UDP socket");
    }
    if (!hasPeerInfo_) {
        throw SocketException("socket not connected");
    }

    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &peerAddr_.sin_addr, ipStr, sizeof(ipStr)) == nullptr) {
        throw SocketException("getPeerAddress() failed: " + std::string(strerror(errno)));
    }
    return std::string(ipStr);
}
int Socket::getPeerPort() const {
    if (type_ != SocketType::TCP) {
        throw SocketException("getPeerPort called on UDP socket");
    }
    if (!hasPeerInfo_) {
        throw SocketException("socket not connected");
    }

    return ntohs(peerAddr_.sin_port);
}
void Socket::close() {
    if (sockfd_ == INVALID_SOCKET) {
        return;
    }

#ifdef _WIN32
    closesocket(sockfd_);
#else
    ::close(sockfd_);
#endif

    sockfd_ = INVALID_SOCKET;
    connected_ = false;
    bound_ = false;
    hasPeerInfo_ = false;
    memset(&peerAddr_, 0, sizeof(peerAddr_));
}
void Socket::shutdown(int how) {
    if (type_ != SocketType::TCP) {
        throw SocketException("shutdown called on UDP socket");
    }
    if (!connected_) {
        throw SocketException("socket not connected");
    }
    if (how < 0 || how > 2) {
        throw SocketException("invalid shutdown mode");
    }

    if (::shutdown(sockfd_, how) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        throw SocketException("shutdown() failed: " + std::to_string(error));
#else
        throw SocketException("shutdown() failed: " + std::string(strerror(errno)));
#endif
    }
}
void Socket::initializeWinSock() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw SocketException("WSAStartup failed");
    }
#endif
}
void Socket::cleanupWinSock() {
#ifdef _WIN32
    WSACleanup();
#endif
}
std::string Socket::getLastErrorString() {
#ifdef _WIN32
    int error = WSAGetLastError();
    return "Error " + std::to_string(error);
#else
    return std::string(strerror(errno));
#endif
}
bool Socket::isWouldBlock() {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}
Socket::Socket(SOCKET sockfd, SocketType type, const sockaddr_in& peerAddr) {
    sockfd_ = sockfd;
    type_ = type;
    peerAddr_ = peerAddr;
    hasPeerInfo_ = true;
    connected_ = true;
    bound_ = false;
}