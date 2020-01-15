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

#include "copterinfo.h"

CopterInfo::CopterInfo(int id, Qt::GlobalColor color)
{
    mId = id;
    mColor = color;
    mName = "";
    mName.sprintf("Copter %d", mId);
    mTime = 0;
}

int CopterInfo::getId()
{
    return mId;
}

void CopterInfo::setId(int id, bool changeName)
{
    mId = id;

    if (changeName) {
        mName = "";
        mName.sprintf("Copter %d", mId);
    }
}

QString CopterInfo::getName() const
{
    return mName;
}

void CopterInfo::setName(const QString &name)
{
    mName = name;
}

LocPoint &CopterInfo::getLocation()
{
    return mLocation;
}

void CopterInfo::setLocation(const LocPoint &location)
{
    mLocation = location;
}

LocPoint &CopterInfo::getLocationGps()
{
    return mLocationGps;
}

void CopterInfo::setLocationGps(const LocPoint &locationGps)
{
    mLocationGps = locationGps;
}

LocPoint CopterInfo::getApGoal() const
{
    return mApGoal;
}

void CopterInfo::setApGoal(const LocPoint &apGoal)
{
    mApGoal = apGoal;
}

Qt::GlobalColor CopterInfo::getColor() const
{
    return mColor;
}

void CopterInfo::setColor(const Qt::GlobalColor &color)
{
    mColor = color;
}

qint32 CopterInfo::getTime() const
{
    return mTime;
}

void CopterInfo::setTime(const qint32 &time)
{
    mTime = time;
}
