#ifndef SYSTEMCOMMANDEXECUTOR_H
#define SYSTEMCOMMANDEXECUTOR_H

#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include "vesctasks.h"

class SystemCommandExecutor : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SysCmd)
public:
    explicit SystemCommandExecutor(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE int executeCommand(const QString &command) {
        int exitCode = -1;

        auto tree = Group {
            ProcessTaskItem([&](ProcessTask &task) {
                task.setProgram("sh");
                task.setArguments(QStringList() << "-c" << command);
                task.setStdOutCallback([this](const QString &out) {
                    emit processStandardOutput(out);
                });
                task.setStdErrCallback([this](const QString &err) {
                    emit processStandardError(err);
                });
                return SetupResult::Continue;
            }, [&exitCode](const ProcessTask &task, DoneWith) {
                exitCode = task.exitCode();
                return DoneResult::Success;
            })
        };

        runTree(tree);
        return exitCode;
    }

signals:
    void processStandardOutput(QString);
    void processStandardError(QString);

};

#endif // SYSTEMCOMMANDEXECUTOR_H
