#include "pch.h"
#include "ClientSession.h"
#include<vector>

const int RECV_BUFFER_SIZE = 4096;

ClientSession::ClientSession(SOCKET clientSocket) : clientSocket(clientSocket) 
{
	cout << "[Session " << clientSocket << "] Da duoc khoi tao\n";
}

ClientSession::~ClientSession()
{
	cout << "[Session " << clientSocket << "] Da duoc huy\n";
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket); //Dong socket lai
	}
}

void ClientSession::run()
{
	vector<char> buffer(RECV_BUFFER_SIZE);
	int bytesRec;

	try {
		while ((bytesRec = recv(clientSocket, buffer.data(), buffer.size() - 1, 0)) > 0) {
			buffer[bytesRec] = '\0';
			string request(buffer.data());

			cout << "Client: " << request << endl;
		}

		if (bytesRec == 0) {
			cout << "Client ngat ket noi!!!\n";
		}
		else {
			cerr << "[Session] ERROR: recv() that bai!!!\n";
		}
	}
	catch (const exception& e) {
		cerr << "[Session] ERROR: Ngoai le!!!\n";
	}

	delete this;
}
