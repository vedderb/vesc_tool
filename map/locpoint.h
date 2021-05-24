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

#ifndef LOCPOINT_H
#define LOCPOINT_H

#include <QPointF>
#include <QString>
#include <QColor>

class LocPoint
{
public:
    LocPoint(double x = 0, double y = 0, double height = 0, double roll = 0, double pitch = 0, double yaw = 0,
             double speed = 0.5, double radius = -1.0, double sigma = 0.0,
             QColor color = Qt::darkGreen, qint32 time = 0, int id = 0, bool drawLine = true);
    LocPoint(const LocPoint &point);

    double getX() const;
    double getY() const;
    double getHeight() const;
    double getRoll() const;
    double getPitch() const;
    double getYaw() const;
    double getSpeed() const;
    QPointF getPoint() const;
    QPointF getPointMm() const;
    double getRadius() const;
    double getSigma() const;
    QString getInfo() const;
    QColor getColor() const;
    qint32 getTime() const;
    int getId() const;
    bool getDrawLine() const;
    double getDistanceTo(const LocPoint &point) const;
    double getDistanceTo3d(const LocPoint &point) const;

    void setX(double x);
    void setY(double y);
    void setHeight(double height);
    void setXY(double x, double y);
    void scaleXY(double scalefactor);
    void setRoll(double roll);
    void setPitch(double pitch);
    void setYaw(double alpha);
    void setSpeed(double speed);
    void setRadius(double radius);
    void setSigma(double sigma);
    void setInfo(const QString &info);
    void setColor(const QColor &color);
    void setTime(const qint32 &time);
    void setId(int id);
    void setDrawLine(bool drawLine);

    // Operators
    LocPoint& operator=(const LocPoint& point);
    bool operator==(const LocPoint& point);
    bool operator!=(const LocPoint& point);

private:
    double mX;
    double mY;
    double mHeight;
    double mRoll;
    double mPitch;
    double mYaw;
    double mSpeed;
    double mRadius;
    double mSigma;
    QString mInfo;
    QColor mColor;
    qint32 mTime;
    int mId;
    bool mDrawLine;

};

#endif // LOCPOINT_H
