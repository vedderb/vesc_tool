#pragma once

#include <QCompleter>

class QLispCompleter : public QCompleter
{
    Q_OBJECT

public:
    explicit QLispCompleter(QObject* parent=nullptr);

};


