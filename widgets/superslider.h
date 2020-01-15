/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

// NOTE: This is heavily based on
// https://stackoverflow.com/questions/17361885/range-slider-in-qt-two-handles-in-a-qslider

#pragma once

#include <QSlider>
#include <QLabel>

class SuperSliderHandle;

class SuperSlider: public QSlider
{
    Q_OBJECT
public:
    SuperSlider(QWidget *parent = nullptr);

    virtual void resizeEvent(QResizeEvent *event);

    int alt_value();
    void alt_setValue(int value);
    void alt_update();

signals:
    void alt_valueChanged(int);

private:
    SuperSliderHandle *alt_handle;

};

class SliderEventFilter : public QObject
{
public:
    SliderEventFilter(SuperSlider *_grandParent)
    {grandParent = _grandParent;}

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    SuperSlider *grandParent;

};

class SuperSliderHandle: public QLabel
{
    Q_OBJECT
public:
    SuperSliderHandle(SuperSlider *parent = nullptr);

    void mousePressEvent(QMouseEvent *event);

    int value();
    int mapValue();
    void moveAndUptade(QPoint p);

    SuperSlider *parent;

private:
    SliderEventFilter *filter;
    int valueNow;

public slots:
    void setValue(int value);
};
