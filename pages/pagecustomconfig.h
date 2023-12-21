/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef PAGECUSTOMCONFIG_H
#define PAGECUSTOMCONFIG_H

#include <QWidget>
#include "vescinterface.h"

namespace Ui {
class PageCustomConfig;
}

class PageCustomConfig : public QWidget
{
    Q_OBJECT

public:
    explicit PageCustomConfig(QWidget *parent = nullptr);
    ~PageCustomConfig();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void setConfNum(int num);

private slots:
    void customConfigLoadDone();

    void on_readButton_clicked();
    void on_readDefaultButton_clicked();
    void on_writeButton_clicked();
    void on_saveXmlButton_clicked();
    void on_loadXmlButton_clicked();

private:
    Ui::PageCustomConfig *ui;
    VescInterface *mVesc;
    int mConfNum;

};

#endif // PAGECUSTOMCONFIG_H
