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

        auto conn = connect(&process, SIGNAL(finished()), &loop, SLOT(quit()));
        QTimer::singleShot(0, [&, command]() {
            process.start("sh", QStringList() << "-c" << command);
        });

        auto conn2 = connect(&process, &QIODevice::readyRead, [&]() {
            emit processOutput(QString::fromUtf8(process.readAll()));
        });

        loop.exec();
        disconnect(conn);
        disconnect(conn2);

        QString output = process.readAllStandardOutput();
        QString errorOutput = process.readAllStandardError();
        return output + errorOutput;
    }

signals:
    void processOutput(QString);

};

#endif // SYSTEMCOMMANDEXECUTOR_H
