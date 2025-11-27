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