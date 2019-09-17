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

#include "locpoint.h"
#include <cmath>

LocPoint::LocPoint(double x, double y, double height, double roll, double pitch, double yaw, double speed,
                   double radius, double sigma, QColor color, qint32 time, int id, bool drawLine) :
    mX(x), mY(y), mHeight(height), mRoll(roll), mPitch(pitch), mYaw(yaw), mSpeed(speed),
    mRadius(radius), mSigma(sigma), mColor(color), mTime(time), mId(id), mDrawLine(drawLine)
{

}

LocPoint::LocPoint(const LocPoint &point)
{
    *this = point;
}

double LocPoint::getX() const
{
    return mX;
}

double LocPoint::getY() const
{
    return mY;
}

double LocPoint::getHeight() const
{
    return mHeight;
}

double LocPoint::getRoll() const
{
    return mRoll;
}

double LocPoint::getPitch() const
{
    return mPitch;
}

double LocPoint::getYaw() const
{
    return mYaw;
}

double LocPoint::getSpeed() const
{
    return mSpeed;
}

QPointF LocPoint::getPoint() const
{
    return QPointF(mX, mY);
}

QPointF LocPoint::getPointMm() const
{
    return QPointF(mX * 1000.0, mY * 1000.0);
}

double LocPoint::getRadius() const
{
    return mRadius;
}

double LocPoint::getSigma() const
{
    return mSigma;
}

void LocPoint::setX(double x)
{
    mX = x;
}

void LocPoint::setY(double y)
{
    mY = y;
}

void LocPoint::setHeight(double height)
{
    mHeight = height;
}

void LocPoint::setXY(double x, double y)
{
    mX = x;
    mY = y;
}

void LocPoint::setTime(const qint32 &time)
{
    mTime = time;
}

void LocPoint::setId(int id)
{
    mId = id;
}

QString LocPoint::getInfo() const
{
    return mInfo;
}

QColor LocPoint::getColor() const
{
    return mColor;
}

qint32 LocPoint::getTime() const
{
    return mTime;
}

int LocPoint::getId() const
{
    return mId;
}

LocPoint &LocPoint::operator =(const LocPoint &point)
{
    mX = point.mX;
    mY = point.mY;
    mHeight = point.mHeight;
    mRoll = point.mRoll;
    mPitch = point.mPitch;
    mYaw = point.mYaw;
    mSpeed = point.mSpeed;
    mRadius = point.mRadius;
    mSigma = point.mSigma;
    mInfo = point.mInfo;
    mColor = point.mColor;
    mTime = point.mTime;
    mId = point.mId;
    mDrawLine = point.mDrawLine;
    return *this;
}

bool LocPoint::getDrawLine() const
{
    return mDrawLine;
}

double LocPoint::getDistanceTo(const LocPoint &point) const
{
    return sqrt((point.mX - mX) * (point.mX - mX) +
                (point.mY - mY) * (point.mY - mY));
}

double LocPoint::getDistanceTo3d(const LocPoint &point) const
{
    return sqrt((point.mX - mX) * (point.mX - mX) +
                (point.mY - mY) * (point.mY - mY) +
                (point.mHeight - mHeight) * (point.mHeight - mHeight));
}

bool LocPoint::operator ==(const LocPoint &point)
{
    if (    mX == point.mX &&
            mY == point.mY &&
            mHeight == point.mHeight &&
            mRoll == point.mRoll &&
            mPitch == point.mPitch &&
            mYaw == point.mYaw &&
            mSpeed == point.mSpeed &&
            mRadius == point.mRadius &&
            mSigma == point.mSigma &&
            mInfo == point.mInfo &&
            mColor == point.mColor &&
            mTime == point.mTime &&
            mId == point.mId &&
            mDrawLine == point.mDrawLine) {
        return true;
    } else {
        return false;
    }
}

bool LocPoint::operator !=(const LocPoint &point)
{
    return !(operator==(point));
}

void LocPoint::setInfo(const QString &info)
{
    mInfo = info;
}

void LocPoint::setRoll(double roll)
{
    mRoll = roll;
}

void LocPoint::setPitch(double pitch)
{
    mPitch = pitch;
}

void LocPoint::setYaw(double alpha)
{
    mYaw = alpha;
}

void LocPoint::setSpeed(double speed)
{
    mSpeed = speed;
}

void LocPoint::setRadius(double radius)
{
    mRadius = radius;
}

void LocPoint::setSigma(double sigma)
{
    mSigma = sigma;
}

void LocPoint::setColor(const QColor &color)
{
    mColor = color;
}

void LocPoint::setDrawLine(bool drawLine)
{
    mDrawLine = drawLine;
}
