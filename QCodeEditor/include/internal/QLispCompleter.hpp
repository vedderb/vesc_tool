#pragma once

#include <QCompleter>

class QLispCompleter : public QCompleter
{
    Q_OBJECT

public:
    explicit QLispCompleter(QObject* parent=nullptr);

protected:
    QStringList splitPath(const QString &path) const override;
    QString pathFromIndex(const QModelIndex &index) const override;

};


