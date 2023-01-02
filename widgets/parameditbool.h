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

#ifndef PARAMEDITBOOL_H
#define PARAMEDITBOOL_H

#include <QWidget>
#include "configparams.h"

namespace Ui {
class ParamEditBool;
}

class ParamEditBool : public QWidget
{
    Q_OBJECT

public:
    explicit ParamEditBool(QWidget *parent = 0);
    ~ParamEditBool();
    void setConfig(ConfigParams *config);
    QString name() const;
    void setName(const QString &name);

private slots:
    void paramChangedBool(QObject *src, QString name, bool newParam);

    void on_readButton_clicked();
    void on_readDefaultButton_clicked();
    void on_helpButton_clicked();
    void on_valueBox_currentIndexChanged(int index);

private:
    Ui::ParamEditBool *ui;
    ConfigParams *mConfig;
    QString mName;
};

#endif // PARAMEDITBOOL_H
