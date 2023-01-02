/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#ifndef PARAMEDITBITFIELD_H
#define PARAMEDITBITFIELD_H

#include <QWidget>
#include "configparams.h"

namespace Ui {
class ParamEditBitfield;
}

class ParamEditBitfield : public QWidget
{
    Q_OBJECT

public:
    explicit ParamEditBitfield(QWidget *parent = nullptr);
    ~ParamEditBitfield();

    void setConfig(ConfigParams *config);
    QString name() const;
    void setName(const QString &name);

private slots:
    void paramChangedInt(QObject *src, QString name, int newParam);

    void on_readButton_clicked();
    void on_readDefaultButton_clicked();
    void on_helpButton_clicked();

private:
    Ui::ParamEditBitfield *ui;
    ConfigParams *mConfig;
    QString mName;

    void setBits(int num);
    int getNum();

};

#endif // PARAMEDITBITFIELD_H
