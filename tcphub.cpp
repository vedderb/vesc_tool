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
    //mPeriodic->start(1000);
}

void TcpHub::addVesc(QString id, TcpConnectedVesc *vesc)
{
    mTcpConnectedVescs.insert(id, vesc);
}

void TcpHub::addTool(QString id, TcpConnectedTool *tool)
{
    mTcpConnectedTools.insert(id, tool);
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

    TcpHubRouter *router = nullptr;
    if (mTcpHubRouters.contains(addr.toString())) {
        router = mTcpHubRouters[addr.toString()];
        router->connect_client(socket);
        if (router->mTcpSocket) {
            router->mTcpSocket->close();
            delete router->mTcpSocket;
            router->mTcpSocket = socket;
        }
    } else {
        router = new TcpHubRouter(this);
        router->mTcpSocket = socket;
        mTcpHubRouters.insert(addr.toString(), router);
        router->connect_client(socket);
    }
    connect(router->mTcpSocket, SIGNAL(readyRead()), router, SLOT(socketReadyRead()));
    connect(router->mTcpSocket, SIGNAL(disconnected()), router, SLOT(socketDisconnected()));
    connect(router, SIGNAL(disconnected(QString)), this, SLOT(routerDisconnected(QString)));
    connect(router, SIGNAL(vescConnection(QTcpSocket*)), this, SLOT(newVescConnection(QTcpSocket*)));
    connect(router, SIGNAL(vescToolConnection(QTcpSocket*)), this, SLOT(newVescToolConnection(QTcpSocket*)));
}

void TcpHub::routerDisconnected(QString addr)
{
    mTcpHubRouters.remove(addr);
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

void TcpHub::newVescToolConnection(QTcpSocket *socket)
{
    QString addr = socket->peerAddress().toString();

    if (mTcpConnectedTools.contains(addr)) {  // A reconnect ?
        TcpConnectedTool *tool = mTcpConnectedTools[addr];
        socket->setParent(tool);
        tool->setTcpSocket(socket);
    } else {
        TcpConnectedTool *tool = new TcpConnectedTool(this);
        socket->setParent(tool); // Move ownership of this socket.
        tool->setTcpSocket(socket);
        mTcpConnectedTools.insert(addr, tool);
    }
}

void TcpHub::periodicTask()
{
    QString t = QDateTime::currentDateTime().toString();
    qDebug() << "Connected tools: " << t;
    foreach (auto e, mTcpConnectedTools.keys()) {
        qDebug() << e;
    }
    qDebug() << "Connected VESCs: " << t;




}
