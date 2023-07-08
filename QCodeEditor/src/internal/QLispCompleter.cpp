/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#include <QLispCompleter>
#include <QLanguage>
#include <QStringListModel>
#include <QFile>
#include <QStandardItemModel>

QLispCompleter::QLispCompleter(QObject *parent) :
    QCompleter(parent)
{
    QStandardItemModel *model = new QStandardItemModel(this);
    QVector<QStandardItem *> parents(10);
    parents[0] = model->invisibleRootItem();
    int level = 0;
    QStringList level0Names;

    auto addItem = [&](QString text) {
        if (level == 0) {
            if (level0Names.contains(text)) {
                return;
            }

            level0Names.append(text);
        }

        QStandardItem *item = new QStandardItem;
        item->setText(text);
        parents[level]->appendRow(item);
        parents[level + 1] = item;

        if (parents.size() <= (level + 1)) {
            parents.resize(parents.size() * 2);
        }
    };

    Q_INIT_RESOURCE(qcodeeditor_resources);
    QFile fl(":/languages/lisp.xml");

    if (fl.open(QIODevice::ReadOnly)) {
        QLanguage language(&fl);

        if (language.isLoaded()) {
            auto keys = language.keys();
            foreach (auto&& key, keys) {
                for (auto name: language.names(key)) {
                    addItem(name);
                }
            }
        }

        fl.close();
    }

    setCompletionMode(QCompleter::PopupCompletion);

    setModel(model);
    setModelSorting(QCompleter::UnsortedModel);
    setCaseSensitivity(Qt::CaseInsensitive);
    setWrapAround(true);
}
