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

#include "superslider.h"

#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QWidgetAction>
#include <QApplication>
#include <QGuiApplication>
#include <QProxyStyle>
#include <QDir>
#include <QPixmap>
#include <QCursor>
#include <QDebug>

class SliderProxy : public QProxyStyle
{
public:
    int pixelMetric (PixelMetric metric, const QStyleOption * option = nullptr, const QWidget * widget = nullptr) const
    {
        switch(metric) {
        case PM_SliderLength     : return 14;
        case PM_SliderThickness  : return 20;
        default                  : return (QProxyStyle::pixelMetric(metric,option,widget));
        }
    }
};

SuperSlider::SuperSlider(QWidget *parent)
    : QSlider(parent)
{
    setOrientation(Qt::Horizontal);
    SliderProxy *aSliderProxy = new SliderProxy();
    aSliderProxy->setParent(this);

    setStyleSheet("QSlider::handle { image: url(://res/images/slider1.png); }"

                  "QSlider::groove:horizontal {"
                  "border: 1px solid #999999;"
                  "height: 20px;"
                  "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);"
                  "margin: 2px 0;}"

                  "QSlider::handle:horizontal {"
                  "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);"
                  "border: 0px solid #5c5c5c;"
                  "width: 14px;"
                  "margin: 0px 0;"
                  "border-radius: 0px;}");

    setStyle(aSliderProxy);
    alt_handle = new SuperSliderHandle(this);
    addAction(new QWidgetAction(alt_handle));
    alt_handle->move(0, this->pos().y());
}

SuperSliderHandle::SuperSliderHandle(SuperSlider *_parent) : QLabel(_parent)
{
    parent = _parent;
    filter = new SliderEventFilter(parent);
    valueNow = 0;

    QPixmap pix = QPixmap("://res/images/slider1.png");
    pix =  pix.scaled(14, 20, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    setPixmap(pix);
}

int SuperSlider::alt_value()
{
    return alt_handle->value();
}

void SuperSlider::alt_setValue(int value)
{
    int valOld = alt_handle->value();
    alt_handle->setValue(value);

    if (valOld != alt_handle->value()) {
        emit alt_valueChanged(alt_value());
    }
}

void SuperSlider::resizeEvent(QResizeEvent *event)
{
    double range = double(maximum() - minimum());
    int location = int(double((event->size().width() - 14) * (alt_handle->value() - minimum())) / range);
    alt_handle->move(location, alt_handle->y());
}

void SuperSlider::alt_update()
{
    QPoint point(alt_handle->mapToParent(alt_handle->mapFromGlobal(QCursor::pos())).x(), alt_handle->y());
    int w = (alt_handle->width());
    point.setX(point.x() - w / 2);

    if (mapToParent(point).x() > pos().x() + width() - w) {
        point.setX(mapFromParent(pos()).x() + width() - w);
    }
    if (mapToParent(point).x() < pos().x()) {
        point.setX(mapFromParent(pos()).x());
    }

    alt_handle->moveAndUptade(point);
    emit alt_valueChanged(alt_value());
}

void SuperSliderHandle::mousePressEvent(QMouseEvent *mouseEvent)
{
    (void)mouseEvent;
    qGuiApp->installEventFilter(filter);
    parent->clearFocus();
}

bool SliderEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type()) {
    case QEvent::MouseButtonRelease:
        qGuiApp->removeEventFilter(this);
        return true;
    case QEvent::MouseMove:
        grandParent->alt_update();
        return true;
    default:
        return QObject::eventFilter(obj, event);
    }
}

void SuperSliderHandle::setValue(int value)
{
    if (value < parent->minimum()) {
        value = parent->minimum();
    } else if (value > parent->maximum()) {
        value = parent->maximum();
    }

    valueNow = value;

    double range = double(parent->maximum() - parent->minimum());
    int location = int(double((parent->width() - 14) * (value - parent->minimum())) / range);
    move(location, y());
}

int SuperSliderHandle::value()
{
    return valueNow;
}

void SuperSliderHandle::moveAndUptade(QPoint p)
{
    move(p);
    double width = parent->width() - 14;
    double value = pos().x() / width;
    double range = parent->maximum() - parent->minimum();
    valueNow = int(parent->minimum() + (value * range));
}
