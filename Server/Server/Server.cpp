// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include<iostream>
#include "ClientSession.h"
#include<thread>
#include<stdexcept>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

class SocketHelper {
public:
    SocketHelper() {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    #endif
    }
    ~SocketHelper() {
    #ifdef _WIN32
        WSACleanup();
    #endif
    }
};

Server::Server(int port) : port(port), listenSocket(INVALID_SOCKET) {};

Server::~Server() {
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }
}

void Server::run() {
    //Creating the Server Socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //tao 1 socket voi giao thuc la TCP
    if (listenSocket == INVALID_SOCKET) {
        cerr << "[Server] ERROR: KHONG THE KHOI TAO SOCKET!!!\n" << endl;
        return;
    }

    //Defining Server Address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    cout << "[Server]: Gan dia chi (bind) vao port " << this->port << "..." << endl;
    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[Server] ERROR: bind() that bai!!!\n";
        closesocket(listenSocket);
        return;
    }

    //Listen for Incoming Connections
    cout << "[Server]: Chuyen sang trang thai nghe (listen) ...\n";
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "[Server] ERROR: listen() that bai!!!\n";
        closesocket(listenSocket);
        return;
    }

    cout << "[Server]: Cho ket noi tu client ...\n";
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);

        //Accept Client Connection
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            cerr << "[Server] ERROR: accept() that bai!!!\n";
            continue; //Bo qua loi, tiep tuc cho ket noi khac
        }

        cout << "[Server] Da co ket noi. Tao luong ClientSession ...\n";

        ClientSession* session = new ClientSession(clientSocket);

        thread clientThread(&ClientSession::run, session);

        clientThread.detach();
    }
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: code your application's behavior here.
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: code your application's behavior here.
            const int SERVER_PORT = 5555;
            try {
                SocketHelper wsHelper;

                Server server(SERVER_PORT);

                server.run();

            }
            catch (const exception& e) {
                cerr << "ERROR: " << e.what() << endl;
                nRetCode = 1;
            }
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}
