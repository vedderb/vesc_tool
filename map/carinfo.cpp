/*
    Copyright 2012 Benjamin Vedder	benjamin@vedder.se

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

#include "carinfo.h"

CarInfo::CarInfo(int id, Qt::GlobalColor color)
{
    mId = id;
    mColor = color;
    mName = "";
    mName.sprintf("Car %d", mId);
    mTime = 0;
    mLength = 0.8;
    mWidth = 0.335;
    mCornerRadius = 0.02;
}

int CarInfo::getId()
{
    return mId;
}

void CarInfo::setId(int id, bool changeName)
{
    mId = id;

    if (changeName) {
        mName = "";
        mName.sprintf("Car %d", mId);
    }
}

QString CarInfo::getName() const
{
    return mName;
}

void CarInfo::setName(QString name)
{
    mName = name;
}

void CarInfo::setLocation(LocPoint &point)
{
    mLocation = point;
}

LocPoint &CarInfo::getLocationGps()
{
    return mLocationGps;
}

void CarInfo::setLocationGps(LocPoint &point)
{
    mLocationGps = point;
}

Qt::GlobalColor CarInfo::getColor() const
{
    return mColor;
}

void CarInfo::setColor(Qt::GlobalColor color)
{
    mColor = color;
}

LocPoint &CarInfo::getApGoal()
{
    return mApGoal;
}

void CarInfo::setApGoal(const LocPoint &apGoal)
{
    mApGoal = apGoal;
}

qint32 CarInfo::getTime() const
{
    return mTime;
}

void CarInfo::setTime(const qint32 &time)
{
    mTime = time;
}

double CarInfo::getLength() const
{
    return mLength;
}

void CarInfo::setLength(double length)
{
    mLength = length;
}

double CarInfo::getWidth() const
{
    return mWidth;
}

void CarInfo::setWidth(double width)
{
    mWidth = width;
}

double CarInfo::getCornerRadius() const
{
    return mCornerRadius;
}

void CarInfo::setCornerRadius(double cornerRadius)
{
    mCornerRadius = cornerRadius;
}

LocPoint &CarInfo::getLocationUwb()
{
    return mLocationUwb;
}

void CarInfo::setLocationUwb(const LocPoint &locationUwb)
{
    mLocationUwb = locationUwb;
}

LocPoint &CarInfo::getLocation()
{
    return mLocation;
}
