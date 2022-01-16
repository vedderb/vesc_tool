#pragma once

#include <QCompleter>

class QVescCompleter : public QCompleter
{
    Q_OBJECT

public:
    explicit QVescCompleter(QObject* parent=nullptr);

protected:
    QStringList splitPath(const QString &path) const override;
    QString pathFromIndex(const QModelIndex &index) const override;

};


