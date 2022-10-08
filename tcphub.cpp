#include "tcphub.h"
#include <QDateTime>

TcpHub::TcpHub(QObject *parent)
    : QObject{parent}
{
    mTcpHubServer = new QTcpServer(this);
    connect(mTcpHubServer, SIGNAL(newConnection()), this, SLOT(newTcpHubConnection()));

    //mPeriodic = new QTimer(this);
    //mPeriodic->setInterval(2000);
    //connect(mPeriodic, SIGNAL(timeout), this, SLOT(periodicTask()));
    //mPeriodic->start();
}

void TcpHub::addVesc(QString id, TcpConnectedVesc *vesc)
{
    mTcpConnectedVescs.insert(id, vesc);
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

void TcpHub::newVescConnection(QTcpSocket *socket)
{
    TcpConnectedVesc *vesc = nullptr;
    QString addr = socket->peerAddress().toString();
    if (mTcpConnectedVescs.contains(addr)) {
        vesc = mTcpConnectedVescs[addr];
        socket->setParent(vesc);
        vesc->setTcpSocket(socket);
    } else {
        vesc = new TcpConnectedVesc(this);
        socket->setParent(vesc);
        vesc->setTcpSocket(socket);
        mTcpConnectedVescs.insert(addr,vesc);
    }
}

