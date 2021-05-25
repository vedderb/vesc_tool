/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#ifndef PAGEMOTORINFO_H
#define PAGEMOTORINFO_H

#include <QWidget>
#include "vescinterface.h"

namespace Ui {
class PageMotorInfo;
}

class PageMotorInfo : public QWidget
{
    Q_OBJECT

public:
    explicit PageMotorInfo(QWidget *parent = nullptr);
    ~PageMotorInfo();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);
    void reloadParams();

private slots:
    void savingXml();
    void paramChangedQString(QObject *src, QString name, QString newParam);

private slots:
    void on_descriptionHelpButton_clicked();

private:
    Ui::PageMotorInfo *ui;
    VescInterface *mVesc;

};

#endif // PAGEMOTORINFO_H
