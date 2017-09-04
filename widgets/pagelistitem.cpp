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

#include "pagelistitem.h"
#include <QHBoxLayout>

PageListItem::PageListItem(QString name,
                           QString icon,
                           QString groupIcon,
                           QWidget *parent) : QWidget(parent)
{
    mIconLabel = new QLabel;
    mNameLabel = new QLabel;
    mGroupLabel = new QLabel;
    mSpaceStart = new QSpacerItem(2, 0);

    mIconLabel->setScaledContents(true);
    mGroupLabel->setScaledContents(true);

    setName(name);
    setIcon(icon);
    setGroupIcon(groupIcon);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);

    layout->addSpacerItem(mSpaceStart);
    layout->addWidget(mIconLabel);
    layout->addWidget(mNameLabel);
    layout->addStretch();
    layout->addWidget(mGroupLabel);
    layout->addSpacing(2);

    this->setLayout(layout);
}

void PageListItem::setName(const QString &name)
{
    mNameLabel->setText(name);
}

void PageListItem::setIcon(const QString &path)
{
    if (!path.isEmpty()) {
        mIconLabel->setPixmap(QPixmap(path));
        mIconLabel->setFixedSize(15, 15);
    } else {
        mIconLabel->setPixmap(QPixmap());
    }
}

void PageListItem::setGroupIcon(const QString &path)
{
    if (!path.isEmpty()) {
        QPixmap pix(path);
        mGroupLabel->setPixmap(pix);
        mGroupLabel->setFixedSize((15 * pix.width()) / pix.height(), 15);
    } else {
        mSpaceStart->changeSize(2, 0);
        mGroupLabel->setPixmap(QPixmap());
    }
}

QString PageListItem::name()
{
    return mNameLabel->text();
}

void PageListItem::setBold(bool bold)
{
    QFont f = mNameLabel->font();
    f.setBold(bold);
    mNameLabel->setFont(f);
}

void PageListItem::setIndented(bool indented)
{
    mSpaceStart->changeSize(indented ? 15 : 2, 0);
}
