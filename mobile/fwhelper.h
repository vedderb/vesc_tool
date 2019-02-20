/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

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

#ifndef FWHELPER_H
#define FWHELPER_H

#include <QObject>
#include <QVariantMap>
#include "vescinterface.h"

class FwHelper : public QObject
{
    Q_OBJECT
public:
    explicit FwHelper(QObject *parent = nullptr);
    Q_INVOKABLE QVariantMap getHardwares(QString hw = "");
    Q_INVOKABLE QVariantMap getFirmwares(QString hw);
    Q_INVOKABLE QVariantMap getBootloaders(QString hw);
    Q_INVOKABLE bool uploadFirmware(QString filename, VescInterface *vesc,
                                    bool isBootloader, bool isIncluded, bool fwdCan);

signals:

public slots:
};

#endif // FWHELPER_H
