#pragma once
#include "resource.h"

//Xu li da nen tang
#ifdef _WIN32
#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
using SOCKET = int;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1
#define closesocket close
#endif

class Server {
private:
	SOCKET listenSocket;
	int port;
public:
	Server(int);
	~Server();
	void run();
};
