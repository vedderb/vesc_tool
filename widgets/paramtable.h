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

#ifndef PARAMTABLE_H
#define PARAMTABLE_H

#include <QWidget>
#include <QTableWidget>
#include <QPointer>
#include "configparams.h"

class ParamTable : public QTableWidget
{
    Q_OBJECT
public:
    ParamTable(QWidget *parent = nullptr);
    bool addParamRow(ConfigParams *params, QString paramName);
    void addRowSeparator(QString text);
    void addParamSubgroup(ConfigParams *params, QString groupName, QString subgroupName);
    void clearParams();
    bool hasChanges();

private:
    // Parameter shown in each row, empty for separators
    QVector<QPair<QPointer<ConfigParams>, QString>> mRows;

    bool rowChanged(int row);
    QString referenceString(int row);
    void updateChanged();
    void showContextMenu(const QPoint &pos);
    void revertRow(int row);

};

#endif // PARAMTABLE_H
