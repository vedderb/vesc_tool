#ifndef BOARDSETUP_H
#define BOARDSETUP_H

#include <QWidget>

namespace Ui {
class boardsetup;
}

class boardsetup : public QWidget
{
    Q_OBJECT

public:
    explicit boardsetup(QWidget *parent = nullptr);
    ~boardsetup();

private:
    Ui::boardsetup *ui;
};

#endif // BOARDSETUP_H
