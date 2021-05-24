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

#ifndef COPTERINFO_H
#define COPTERINFO_H

#include <QVector>
#include <QString>
#include "locpoint.h"

class CopterInfo
{
public:
    CopterInfo(int id = 0, Qt::GlobalColor color = Qt::red);
    int getId();
    void setId(int id, bool changeName = false);
    QString getName() const;
    void setName(const QString &name);
    LocPoint &getLocation();
    void setLocation(const LocPoint &location);
    LocPoint &getLocationGps();
    void setLocationGps(const LocPoint &locationGps);
    LocPoint getApGoal() const;
    void setApGoal(const LocPoint &apGoal);
    Qt::GlobalColor getColor() const;
    void setColor(const Qt::GlobalColor &color);
    qint32 getTime() const;
    void setTime(const qint32 &time);

private:
    int mId;
    QString mName;
    LocPoint mLocation;
    LocPoint mLocationGps;
    LocPoint mApGoal;
    Qt::GlobalColor mColor;
    qint32 mTime;

};

#endif // COPTERINFO_H
