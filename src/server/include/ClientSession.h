#pragma once
#include "resource.h"
#include<string>

using namespace std;

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

class ClientSession {
private:
	SOCKET clientSocket; //dung de giao tiep voi client
public:
	ClientSession(SOCKET clientSocket);
	virtual ~ClientSession();
	void run();
};