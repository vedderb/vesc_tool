#ifndef SYSTEMCOMMANDEXECUTOR_H
#define SYSTEMCOMMANDEXECUTOR_H

#include <QObject>
#include <QProcess>

class SystemCommandExecutor : public QObject
{
    Q_OBJECT
public:
    explicit SystemCommandExecutor(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString executeCommand(const QString &command) {
        QProcess process;
        process.start("sh", QStringList() << "-c" << command);
        process.waitForFinished(-1); // wait indefinitely for the process to finish
        QString output = process.readAllStandardOutput();
        QString errorOutput = process.readAllStandardError();
        return output + errorOutput;
    }
};

#endif // SYSTEMCOMMANDEXECUTOR_H
