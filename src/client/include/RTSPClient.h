#ifndef RTSPCLIENT_H
#define RTSPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QHostAddress>

class RTSPClient : public QObject
{
    Q_OBJECT

public:
    enum State
    {
        INIT,
        READY,
        PLAYING
    };
    explicit RTSPClient(QObject *parent = nullptr);
    ~RTSPClient(); // Default destructor
    void connectToServer(const QString &host, quint16 port, const QString &filename, quint16 rtpPort);
    // Các lệnh RTSP
    void setup();
    void play();
    void pause();
    void teardown();

    // Getter
    State currentState() const { return m_state; }

signals:
    void connectedToHost();
    void connectionError(const QString &error);

    void setupSuccess(const QString &sessionId);
    void playSuccess();
    void pauseSuccess();
    void teardownSuccess();

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QTcpSocket::SocketError socketError);

private:
    QTcpSocket *m_tcpSocket;
    State m_state;
    int m_cSeq;
    QString m_sessionId;
    QString m_fileName;
    quint16 m_rtpClientPort;
    QString m_host;
    quint16 m_rtspPort;

    QString buildRTSPRequest(const QString &method);
    bool parseRTSPResponse(const QString &response);
};

#endif