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

    Q_INVOKABLE int executeCommand(const QString &command) {
        QProcess process;
        QEventLoop loop;

        process.setReadChannel(QProcess::StandardOutput);

        auto quitHandler = connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
        auto errHandler = connect(&process, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
        auto stdOutHandler = connect(&process, &QProcess::readyReadStandardOutput, [&]() {
            emit processStandardOutput(QString::fromUtf8(process.readAllStandardOutput()));
        });
        auto stdErrHandler = connect(&process, &QProcess::readyReadStandardError, [&]() {
            emit processStandardError(QString::fromUtf8(process.readAllStandardError()));
        });

        process.start("sh", QStringList() << "-c" << command);
        
        if (!process.waitForStarted()) {
            disconnect(quitHandler);
            disconnect(errHandler);
            disconnect(stdOutHandler);
            disconnect(stdErrHandler);
            return -1;
        }

        loop.exec();

        disconnect(quitHandler);
        disconnect(errHandler);
        disconnect(stdOutHandler);
        disconnect(stdErrHandler);

        return process.exitCode();
    }

signals:
    void processStandardOutput(QString);
    void processStandardError(QString);

};

#endif // SYSTEMCOMMANDEXECUTOR_H
