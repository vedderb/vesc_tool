/*
    Copyright 2019 - 2023 Benjamin Vedder	benjamin@vedder.se

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

#ifndef IMAGEWIDGETDISP_H
#define IMAGEWIDGETDISP_H

#include <QWidget>

class ImageWidgetDisp : public QWidget
{
    Q_OBJECT
public:
    explicit ImageWidgetDisp(QWidget *parent = 0);

    QPixmap pixmap() const;
    void setPixmap(const QPixmap &pixmap);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:
    QPixmap mPixmap;

};

#endif // IMAGEWIDGETDISP_H
