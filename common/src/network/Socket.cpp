#include "Socket.hpp"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif 

Socket::Socket(SocketType socketType)
{
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
    if(sockfd_ == INVALID_SOCKET) {
        throw SocketException("socket failed");
    }

    type_ = socketType;
    connected_ = false;
    bound_ = false;
    hasPeerInfo_ = false;

    memset(&peerAddr_, 0, sizeof(peerAddr_));
}
~Socket()
{
    #ifdef _WIN32
        WSACleanup();
    #endif
    if(sockfd_ != INVALID_SOCKET) {
        closesocket(sockfd_);
    }
}
Socket::Socket(Socket &&other) noexcept
{
    *this = std::move(other);
}
Socket::&operator=(Socket &&other) noexcept
{
    if(this != &other) {
        close();
        Socket(other);
    }
    return *this;
}
void Socket::bind(const std::string &address, int port)
{
    if(port < 0 || port > 65535) {
        throw SocketException("invalid port");
    }

    if(bound_) {
        throw SocketException("socket already bound");
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if(bind(sockfd_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        throw SocketException("bind failed");
    }
    bound_ = true;
}
void Socket::listen(int backlog)
{
    if(type_ != SocketType::TCP) {
        throw SocketException("listen called on UDP socket");
    }
    if(!bound_) {
        throw SocketException("socket not bound");
    }
    
    if(backlog <= 0){
        throw SocketException("invalid backlog");
    }

    if(::listen(sockfd_, backlog) == SOCKET_ERROR) {
        throw SocketException("listen failed");
    }
}
std::unique_ptr<Socket> Socket::accept()
{
    if(type_ != SocketType::TCP) {
        throw SocketException("accept called on UDP socket");
    }
    if(!bound_) {
        throw SocketException("socket not bound");
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrLen = sizeof(addr);

    // 3. Gọi system call accept() (BLOCKING - chờ đến khi có client)
    SOCKET clientSocket = ::accept(sockfd_, (sockaddr*)&addr, &addrLen);

    if(clientSocket == INVALID_SOCKET) {
        #ifdef _WIN32
            int error = WSAGetLastError();
            if(error == WSAETIMEDOUT) {
                throw SocketTimeout("accept timed out");
            }
        #else
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                throw SocketTimeout("accept timed out");
            }
        #endif
        throw SocketException("accept failed");
    }

    return std::make_unique<Socket>(clientSocket, type_, addr);
}
void Socket::connect(const std::string &host, int port)
{
    if(type_ != SocketType::TCP) {
        throw SocketException("connect called on UDP socket");
    }
    if(connected_) {
        throw SocketException("socket already connected");
    }
    if(port < 0 || port > 65535) {
        throw SocketException("invalid port");
    }
    
    struct addrinfo hints, *result = nullptr, *rp = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr = std::to_string(port);
    int ret = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if(ret != 0) {
        #ifdef _WIN32
            throw SocketException("DNS lookup failed for host: " + host + 
            "(error: " + std::to_string(ret) + ")");
        #else
            throw SocketException("DNS lookup failed for host: " + host + 
            "(" + std::string(gai_strerror(ret)) + ")");
        #endif
    }
    
    bool connected = false;
    for(rp = result; rp != nullptr; rp = rp->ai_next){
        //* Try to connect to the address
        if(::connect(sockfd_, rp->ai_addr, rp->ai_addrlen) == 0) {
            connected = true;
            
            memcpy(&peerAddr_, rp->ai_addr, rp->ai_addrlen);
            hasPeerInfo_ = true;
            break;
        }
    }

    freeaddrinfo(result);

    if(!connected) {
        #ifdef _WIN32
            int error = WSAGetLastError();
            std::string errorMsg;
            
            switch(error) {
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
            switch(errno) {
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

        connected_ = true;
    }
}
int Socket::send(const uint8_t *data, size_t length)
{
    if(type_ != SocketType::TCP) {
        throw SocketException("send called on UDP socket");
    }
    if(!connected_) {
        throw SocketException("socket not connected");
    }

    if(length <= 0 || data == nullptr) {
        throw SocketException("invalid length");
    }

    int totalSent = 0;
    while(totalSent < length) {
        int bytes = ::send(sockfd_, 
            (const char*)(data + totalSent,) 
            length - totalSent, 
            0);

        if(bytes == SOCKET_ERROR) {
            #ifdef _WIN32
                int error = WSAGetLastError();
                if(error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                    throw SocketTimeout("send() timed out");
                }
                throw SocketException("send() failed: " + std::to_string(error));
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    throw SocketTimeout("send() timed out");
                }
                throw SocketException("send() failed: " + std::string(strerror(errno)));
            #endif
        }

        if(bytes == 0) {
            throw SocketException("send() failed: connection closed by peer");
        }

        totalSent += bytes;
    }

    return totalSent;
}
int Socket::receive(uint8_t *buffer, size_t bufferSize)
{
    if(type_ != SocketType::TCP){
        throw SocketException("receive called on UDP socket");
    }
    if(!connected_){
        throw SocketException("socket not connected");
    }

    if(bufferSize <= 0 || buffer == nullptr){
        throw SocketException("invalid buffer size");
    }

    int bytesReceived = ::recv(sockfd_, (char*)buffer, bufferSize, 0);

    if(bytesReceived == SOCKET_ERROR){
        #ifdef _WIN32
            int error = WSAGetLastError();
            if(error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                throw SocketTimeout("receive() timed out");
            }
            throw SocketException("receive() failed: " + std::to_string(error));
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                throw SocketTimeout("receive() timed out");
            }
        #endif
        throw SocketException("receive() failed: " + std::string(strerror(errno)));
    }

    return bytesReceived;
}
int Socket::sendTo(const uint8_t *data, size_t length,
                    const std::string &destAddress, int destPort)
{
    if(type_ != SocketType::UDP){
        throw SocketException("sendTo called on TCP socket");
    }
    if(!bound_){
        throw SocketException("socket not bound");
    }
    if(length <= 0 || data == nullptr){
        throw SocketException("invalid length");
    }

    if(destPort < 0 || destPort > 65535){
        throw SocketException("invalid destination port");
    }

    sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destPort);

    //* Convert destination address from string to binary
    if(inet_pton(AF_INET, destAddress.c_str(), &destAddr.sin_addr) == 0){
        throw SocketException("invalid destination address");
    }

    int bytesSent = ::sendto(sockfd_, 
                            (const char*)(data + totalSent), 
                            length - totalSent, 
                            0,
                            (struct sockaddr*)&destAddr, 
                            sizeof(destAddr));

    if(bytesSent == SOCKET_ERROR){
        #ifdef _WIN32
            int error = WSAGetLastError();
            throw SocketException("sendTo() failed: " + to_string(error));
        #else
            throw SocketException("sendTo() failed: " + std::string(strerror(errno)));
        #endif
    }
    return bytesSent;
}
int Socket::receiveFrom(uint8_t *buffer, size_t bufferSize,
                        std::string &sourceAddress, int &sourcePort)
{
    if(type_ != SocketType::UDP){
        throw SocketException("receiveFrom called on TCP socket");
    }
    if(!bound_){
        throw SocketException("socket not bound");
    }
    if(bufferSize <= 0 || buffer == nullptr){
        throw SocketException("invalid buffer size");
    }

    sockaddr_in sourceAddr;
    memset(&sourceAddr, 0, sizeof(sourceAddr));
    socklen_t sourceAddrLen = sizeof(sourceAddr);

    int bytesReceived = ::recvfrom(sockfd_, 
                                (char*)buffer, 
                                bufferSize, 
                                0,
                                (struct sockaddr*)&sourceAddr, 
                                &sourceAddrLen);

    if(bytesReceived == SOCKET_ERROR){
        #ifdef _WIN32
            int error = WSAGetLastError();
            if(error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                throw SocketTimeout("receiveFrom() timed out");
            }
            throw SocketException("receiveFrom() failed: " + std::to_string(error));
        #else
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                throw SocketTimeout("receiveFrom() timed out");
            }
            throw SocketException("receiveFrom() failed: " + std::string(strerror(errno)));
        #endif
    }

    char ipStr[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, &sourceAddr.sin_addr, ipStr, sizeof(ipStr)) == nullptr){
        throw SocketException("receiveFrom() failed: " + std::string(strerror(errno)));
    }
    sourceAddress = std::string(ipStr);
    sourcePort = ntohs(sourceAddr.sin_port);

    return bytesReceived;
}
void Socket::setTimeout(int milliseconds)
{
    if(milliseconds < 0){
        throw SocketException("invalid timeout");
    }

    #ifdef _WIN32
        DWORD timeout = milliseconds;
        if(setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
            throw SocketException("setTimeout() failed: " + std::string(WSAGetLastErrorString(WSAGetLastError())));
        }
    #else
        struct timeval tv;
        tv.tv_sec = milliseconds / 1000;
        tv.tv_usec = (milliseconds % 1000) * 1000;
        if(setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) == SOCKET_ERROR) {
            throw SocketException("setTimeout() failed: " + std::string(strerror(errno)));
        }
    #endif
}
void Socket::setReuseAddress(bool reuse)
{
    int optVal = reuse ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                    (const char*)&optVal, sizeof(optVal)) == SOCKET_ERROR) {
        #ifdef _WIN32
            int error = WSAGetLastError();
            throw SocketException("setReuseAddress() failed: " + std::to_string(error));
        #else
            throw SocketException("setReuseAddress() failed: " + std::string(strerror(errno)));
        #endif
    }
}
void Socket::setNonBlocking(bool nonBlocking)
{
    int flags = fcntl(sockfd_, F_GETFL, 0);
    if(flags == -1){
        throw SocketException("setNonBlocking() failed: " + std::string(strerror(errno)));
    }
    flags = nonBlocking ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
    if(fcntl(sockfd_, F_SETFL, flags) == -1){
        throw SocketException("setNonBlocking() failed: " + std::string(strerror(errno)));
    }
}