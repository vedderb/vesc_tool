/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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

#include "pagebms.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QVBoxLayout>

// The BMS page of the mobile UI, see res/qml/BmsQmlPanel.qml
PageBms::PageBms(QWidget *parent) :
    QWidget(parent)
{
    mVesc = nullptr;
    mUtil = new Utility(this);
    mQml = new QQuickWidget(this);
    mQml->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mQml);
}

VescInterface *PageBms::vesc() const
{
    return mVesc;
}

void PageBms::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    mQml->engine()->rootContext()->setContextProperty("VescIf", mVesc);
    mQml->engine()->rootContext()->setContextProperty("Utility", mUtil);
    mQml->setSource(QUrl(QLatin1String("qrc:/res/qml/BmsQmlPanel.qml")));
}
