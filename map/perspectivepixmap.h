/*
    Copyright 2016 Benjamin Vedder	benjamin@vedder.se

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

#ifndef PERSPECTIVEPIXMAP_H
#define PERSPECTIVEPIXMAP_H

#include <QPixmap>
#include <QPainter>

class PerspectivePixmap
{

public:
    explicit PerspectivePixmap();
    void setPixmap(QPixmap pixmap);
    void setXOffset(double offset);
    void setYOffset(double offset);
    void setScale(double scale);
    QPixmap getPixmap();
    double getXOffset();
    double getYOffset();
    double getScale();
    void drawUsingPainter(QPainter &painter);

signals:
    
public slots:

private:
    QPixmap mPixMap;
    double mXOffset;
    double mYOffset;
    double mScale;
    
};

#endif // PERSPECTIVEPIXMAP_H
