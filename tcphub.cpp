#include "tcphub.h"
#include <QDateTime>

TcpHub::TcpHub(QObject *parent)
    : QObject{parent}
{
    mTcpHubServer = new QTcpServer(this);
    connect(mTcpHubServer, SIGNAL(newConnection()), this, SLOT(newTcpHubConnection()));

    mPeriodic = new QTimer(this);
    connect(mPeriodic, SIGNAL(timeout()), this, SLOT(periodic()));
    mPeriodic->start(1000);
}

bool TcpHub::start(int port, QHostAddress addr)
{
    if (!mTcpHubServer->listen(addr,  port)) {
        return false;
    }
    return true;
}

void TcpHub::newTcpHubConnection()
{
    QTcpSocket *socket = mTcpHubServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::LowDelayOption, true);

    QHostAddress addr = socket->peerAddress();

    qDebug() << "new connection to hub: " << addr.toString();
}

void TcpHub::periodic()
{
    qDebug("periodic");
}

