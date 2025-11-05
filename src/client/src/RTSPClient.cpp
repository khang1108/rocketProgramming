#include "RTSPClient.h"

void RTSPClient::connectToServer(const QString &host, quint16 port, const QString &filename, quint16 rtpPort)
{
    m_tcpSocket->connectToHost(host, port);

    qDebug() << "Trying to connect to server " << host << " with port: " << port;
}
void RTSPClient::onConnected()
{
    qDebug() << "Connect TCP successfully. Ready to SETUP";
    emit connectedToHost();
}