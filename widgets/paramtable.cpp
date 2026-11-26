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
#include "utility.h"
#include <QDebug>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>

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

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &ParamTable::showContextMenu);
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

        mRows.resize(row + 1);
        mRows[row] = {params, paramName};
        connect(params, &ConfigParams::changedParamsChanged, this, &ParamTable::updateChanged, Qt::UniqueConnection);
        updateChanged();

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
    label->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setBold(true);

    label->setStyleSheet("QLabel { background: qlineargradient(x1: 0, y1: 0, x2: 1.0, y2: 0, stop: 0.0" + Utility::getAppHexColor("lightAccent") +
                         ", stop: 0.2 " + Utility::getAppHexColor("darkAccent") +
                         ", stop: 0.8 " + Utility::getAppHexColor("darkAccent") +
                         ", stop: 1.0 " + Utility::getAppHexColor("lightAccent") +");" +
                         " color : white; border: none;}");
    label->setFont(font);
    label->setMargin(3);

    setCellWidget(row, 0, label);
    setSpan(row, 0, 1, 2);

    resizeColumnToContents(0);
    resizeRowsToContents();
}

void ParamTable::addParamSubgroup(ConfigParams *params, QString groupName, QString subgroupName)
{
    setUpdatesEnabled(false);
    foreach (auto p, params->getParamsFromSubgroup(groupName, subgroupName)) {
        if (p.startsWith("::sep::")) {
            addRowSeparator(p.mid(7));
        } else {
            addParamRow(params, p);
        }
    }
    QTimer::singleShot(0, [this]() {
        setUpdatesEnabled(true);
    });
}

void ParamTable::clearParams()
{
    setRowCount(0);
    mRows.clear();
}

bool ParamTable::hasChanges()
{
    for (int row = 0;row < mRows.size();row++) {
        if (rowChanged(row)) {
            return true;
        }
    }
    return false;
}

// The names get room for bold text, so that they still fit when their rows show changes
int ParamTable::sizeHintForColumn(int column) const
{
    int hint = QTableWidget::sizeHintForColumn(column);
    if (column != 0) {
        return hint;
    }

    QFont bold = font();
    bold.setBold(true);
    QFontMetrics fmBold(bold);
    int extra = 0;
    for (int row = 0;row < rowCount();row++) {
        const QTableWidgetItem *it = item(row, 0);
        if (it && !it->font().bold()) {
            extra = qMax(extra, fmBold.horizontalAdvance(it->text()) - fontMetrics().horizontalAdvance(it->text()));
        }
    }

    return hint + extra;
}

bool ParamTable::rowChanged(int row)
{
    const auto &r = mRows.at(row);
    return r.first && r.first->getChangedParams().contains(r.second);
}

// Rows that differ from the device get the accent color and the device value as tooltip
void ParamTable::updateChanged()
{
    QColor accent = Utility::getAppQColor("accentText");
    QColor tint = accent;
    tint.setAlpha(28);

    for (int row = 0;row < mRows.size();row++) {
        QTableWidgetItem *it = item(row, 0);
        if (!it || !mRows.at(row).first) {
            continue;
        }

        bool changed = rowChanged(row);
        QFont f = font();
        f.setBold(changed);
        it->setFont(f);
        it->setData(Qt::ForegroundRole, changed ? QVariant(accent) : QVariant());
        it->setData(Qt::BackgroundRole, changed ? QVariant(tint) : QVariant());
        it->setToolTip(changed ? tr("Value on the VESC: %1").arg(referenceString(row)) : QString());
    }

    // Dot on the tab that holds this table
    for (QWidget *w = this;w->parentWidget();w = w->parentWidget()) {
        QTabWidget *tabs = qobject_cast<QTabWidget*>(w->parentWidget()->parentWidget());
        int ind = tabs ? tabs->indexOf(w) : -1;
        if (ind >= 0) {
            bool changed = hasChanges();
            for (ParamTable *t: w->findChildren<ParamTable*>()) {
                changed = changed || t->hasChanges();
            }
            tabs->setTabIcon(ind, changed ? QIcon(Utility::getChangedDot()) : QIcon());
            break;
        }
    }
}

QString ParamTable::referenceString(int row)
{
    return mRows.at(row).first->getReference()->getParamValueString(mRows.at(row).second);
}

void ParamTable::showContextMenu(const QPoint &pos)
{
    int row = rowAt(pos.y());
    int changedRows = 0;
    for (int i = 0;i < mRows.size();i++) {
        changedRows += rowChanged(i);
    }

    QMenu menu(this);
    if (row >= 0 && row < mRows.size() && rowChanged(row)) {
        menu.addAction(tr("Revert to %1").arg(referenceString(row)), this, [this, row]() { revertRow(row); });
    }

    if (changedRows > 1) {
        menu.addAction(tr("Revert all %1 changes here").arg(changedRows), this, [this]() {
            for (int i = 0;i < mRows.size();i++) {
                revertRow(i);
            }
        });
    }

    if (!menu.isEmpty()) {
        menu.exec(viewport()->mapToGlobal(pos));
    }
}

void ParamTable::revertRow(int row)
{
    if (rowChanged(row)) {
        ConfigParams *params = mRows.at(row).first;
        QString name = mRows.at(row).second;
        params->updateParamFromOther(name, params->getReference()->getParamCopy(name), nullptr);
    }
}
