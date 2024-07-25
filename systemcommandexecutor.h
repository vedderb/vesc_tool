#ifndef SYSTEMCOMMANDEXECUTOR_H
#define SYSTEMCOMMANDEXECUTOR_H

#include <QObject>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>

class SystemCommandExecutor : public QObject
{
    Q_OBJECT
public:
    explicit SystemCommandExecutor(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString executeCommand(const QString &command) {
        QProcess process;
        QEventLoop loop;

        process.setReadChannel(QProcess::StandardOutput);

        auto conn1 = connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
        auto conn2 = connect(&process, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
        auto conn3 = connect(&process, &QProcess::readyReadStandardOutput, [&]() {
            emit processOutput(QString::fromUtf8(process.readAllStandardOutput()));
        });
        auto conn4 = connect(&process, &QProcess::readyReadStandardError, [&]() {
            emit processOutput(QString::fromUtf8(process.readAllStandardError()));
        });

        process.start("sh", QStringList() << "-c" << command);
        
        if (!process.waitForStarted()) {
            disconnect(conn1);
            disconnect(conn2);
            disconnect(conn3);
            disconnect(conn4);
            return "Failed to start process";
        }

        loop.exec();

        disconnect(conn1);
        disconnect(conn2);
        disconnect(conn3);
        disconnect(conn4);

        QString output = process.readAllStandardOutput();
        QString errorOutput = process.readAllStandardError();
        return output + errorOutput;
    }

signals:
    void processOutput(QString);

};

#endif // SYSTEMCOMMANDEXECUTOR_H
