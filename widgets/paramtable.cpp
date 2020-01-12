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

#include "paramtable.h"
#include <QDebug>
#include <QHeaderView>
#include <QLabel>

ParamTable::ParamTable(QWidget *parent) : QTableWidget(parent)
{
    setColumnCount(2);

    QStringList headers;
    headers.append("Name");
    headers.append("Edit");

    setHorizontalHeaderLabels(headers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::NoSelection);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
}

bool ParamTable::addParamRow(ConfigParams *params, QString paramName)
{
    bool res = false;
    QWidget *editor = params->getEditor(paramName);
    QString name = params->getLongName(paramName);

    if (editor) {
        int row = rowCount();
        setRowCount(row + 1);
        QTableWidgetItem *item = new QTableWidgetItem(name);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        setItem(row, 0, item);
        setCellWidget(row, 1, editor);
        res = true;

        resizeColumnToContents(0);
        resizeRowsToContents();
    }

    return res;
}

void ParamTable::addRowSeparator(QString text)
{
    int row = rowCount();
    setRowCount(row + 1);

    QLabel *label = new QLabel(text);
    label->setAlignment(Qt::AlignHCenter);
    QFont font;
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet("QLabel { background-color : lightblue; color : black; }");

    setCellWidget(row, 0, label);
    setSpan(row, 0, 1, 2);

    resizeColumnToContents(0);
    resizeRowsToContents();
}

void ParamTable::addParamSubgroup(ConfigParams *params, QString groupName, QString subgroupName)
{
    for (auto p: params->getParamsFromSubgroup(groupName, subgroupName)) {
        if (p.startsWith("::sep::")) {
            addRowSeparator(p.mid(7));
        } else {
            addParamRow(params, p);
        }
    }
}

void ParamTable::clearParams()
{
    setRowCount(0);
}
