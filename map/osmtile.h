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

#ifndef OSMTILE_H
#define OSMTILE_H

#include <QPixmap>

class OsmTile
{
public:
    OsmTile(QPixmap pxm, int zoom = 0, int x = 0, int y = 0);
    OsmTile(int zoom = 0, int x = 0, int y = 0);
    void setZXY(int zoom, int x, int y);

    int zoom() const;
    void setZoom(double zoom);

    int x() const;
    void setX(int x);

    int y() const;
    void setY(int y);

    QPixmap pixmap() const;
    void setPixmap(const QPixmap &pixmap);

    double getNorth();
    double getSouth();
    double getWest();
    double getEast();

    double getWidthTop();

    // Static functions
    static int long2tilex(double lon, int z);
    static int lat2tiley(double lat, int z);
    static double tilex2long(int x, int z);
    static double tiley2lat(int y, int z);
    static double lat2width(double lat, int zoom);

    // Operators
    bool operator==(const OsmTile& tile);
    bool operator!=(const OsmTile& tile);

private:
    QPixmap mPixmap;
    int mZoom;
    int mX;
    int mY;

};

#endif // OSMTILE_H
