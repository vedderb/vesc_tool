/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "canlistitem.h"
#include "utility.h"
#include <QHBoxLayout>

CANListItem::CANListItem(FW_RX_PARAMS p,
                         int ID,
                         bool ok,
                         QWidget *parent) : QWidget(parent)
{
    mIconLabel = new QLabel;
    mNameLabel = new QLabel;
    mIdLabel = new QLabel;
    mSpaceStart = new QSpacerItem(6, 0);

    QString name, icon;
    QString theme = Utility::getThemePath();
    if (ok) {
        if (p.hwType == HW_TYPE_VESC) {
            icon = theme +"icons/motor_side.png";
            if(p.hw.contains("STORMCORE",Qt::CaseInsensitive)){
                name = "<p align=\"right\"> <img src=\"" + theme + "icons/stormcore-96.png\" height = 9/> " +
                        p.hw.remove("STORMCORE_");
            }else if(p.hw.at(0).isDigit() || p.hw.contains("HD",Qt::CaseInsensitive)) {
                name = "<img src=\"" + theme + "icons/vesc-96.png\" height = 9/>"
                     + " " + p.hw.replace("_", " ");
            }else{
                name = p.hw;
            }
        } else if (p.hwType == HW_TYPE_VESC_BMS) {
            icon = theme +"icons/icons8-battery-100.png";
            name = "BMS (" + p.hw + "):";
        } else {
            icon = theme +"icons/Electronics-96.png";
            name = "Device (" + p.hw + "):";
        }
    } else {
        icon = theme +"icons/Help-96.png";
        name = "Unknown: " ;
    }

    mIconLabel->setScaledContents(true);

    setName(name);
    setIcon(icon);
    setID(ID);

    QHBoxLayout *layout = new QHBoxLayout;
    //layout->setMargin(0);

    layout->addSpacerItem(mSpaceStart);
    layout->addWidget(mIconLabel);
    layout->addWidget(mNameLabel);
    layout->addStretch();
    layout->addWidget(mIdLabel);
    layout->addSpacing(4);

    this->setLayout(layout);
}

void CANListItem::setName(const QString &name)
{
    mNameLabel->setText(name);
    QFont f = mNameLabel->font();
    f.setPointSize(this->font().pointSize()*0.9);
    f.setBold(true);
    mNameLabel->setAlignment(Qt::AlignCenter);
    mNameLabel->setFont(f);
}

void CANListItem::setIcon(const QString &path)
{
    if (!path.isEmpty()) {
        mIconLabel->setPixmap(QPixmap(path));

        QFontMetrics fm(this->font());
        int height = fm.height()*1.1;

        mIconLabel->setFixedSize(height, height);
    } else {
        mIconLabel->setPixmap(QPixmap());
    }
}

void CANListItem::setID(int canID)
{
    this->ID = canID;
    QFont f = mIdLabel->font();
    if(canID == -1){
        mIdLabel->setText("local");
        f.setPointSize(this->font().pointSize()*0.75);
    }else{
        mIdLabel->setText(QString::number(canID));
        f.setPointSize(this->font().pointSize()*0.9);
    }
    mIdLabel->setMinimumWidth(30);
    mIdLabel->setFrameStyle(QFrame::Panel);
    f.setBold(true);
    mIdLabel->setAlignment(Qt::AlignCenter);
    mIdLabel->setFont(f);
}

int CANListItem::getID()
{
    return this->ID;
}

QString CANListItem::name()
{
    return mNameLabel->text();
}

void CANListItem::setBold(bool bold)
{
    QFont f = mNameLabel->font();
    f.setBold(bold);
    mNameLabel->setFont(f);
}

void CANListItem::setIndented(bool indented)
{
    mSpaceStart->changeSize(indented ? 15 : 2, 0);
}
