/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

#ifndef POLLMANAGER_H
#define POLLMANAGER_H

#include "vescinterface.h"
#include <QObject>

class PollManager : public QObject
{
    Q_OBJECT
public:
    explicit PollManager(QObject *parent = nullptr, VescInterface *vesc = nullptr);
    Q_INVOKABLE void setVesc(VescInterface *vesc);
    Q_INVOKABLE void pollField(COMM_PACKET_ID id);

signals:

private:
    VescInterface *mVesc;
    QList<COMM_PACKET_ID> mPollList;
    QTimer mPollTimer;

};

#endif // POLLMANAGER_H
