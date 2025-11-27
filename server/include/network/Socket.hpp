#ifndef SOCKET_SERVER_HPP
#define SOCKET_SERVER_HPP

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

};
#endif