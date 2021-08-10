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

#ifndef CANLISTITEM_H
#define CANLISTITEM_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QSpacerItem>
#include <QResizeEvent>
#include "datatypes.h"

class CANListItem : public QWidget
{
    Q_OBJECT
public:
    explicit CANListItem(FW_RX_PARAMS p,
                         int ID,
                         bool ok,
                         QWidget *parent = 0);
    void setName(const QString &name);
    void setIcon(const QString &path);
    void setID(int canID);
    Q_INVOKABLE int getID();
    QString name();
    void setBold(bool bold);
    void setIndented(bool indented);

signals:

public slots:

private:
    QLabel *mIconLabel;
    QLabel *mNameLabel;
    QLabel *mIdLabel;
    int ID;
    QSpacerItem *mSpaceStart;
};

#endif // CANLISTITEM_H
