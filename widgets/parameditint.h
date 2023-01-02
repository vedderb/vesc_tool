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

#ifndef PARAMEDITINT_H
#define PARAMEDITINT_H

#include <QWidget>
#include <QSpinBox>
#include "configparams.h"
#include "displaypercentage.h"

namespace Ui {
class ParamEditInt;
}

class ParamEditInt : public QWidget
{
    Q_OBJECT

public:
    explicit ParamEditInt(QWidget *parent = 0);
    ~ParamEditInt();

    void setConfig(ConfigParams *config);
    QString name() const;
    void setName(const QString &name);
    void setSuffix(const QString &suffix);
    void setShowAsPercentage(bool showAsPercentage);
    void showDisplay(bool show);

private slots:
    void paramChangedInt(QObject *src, QString name, int newParam);
    void percentageChanged(int p);
    void intChanged(int i);

    void on_readButton_clicked();
    void on_readDefaultButton_clicked();
    void on_helpButton_clicked();

private:
    Ui::ParamEditInt *ui;
    ConfigParams *mConfig;
    ConfigParam mParam;
    QString mName;
    int mMaxVal;

    DisplayPercentage *mDisplay;
    QSpinBox *mIntBox;
    QSpinBox *mPercentageBox;

    void updateDisplay(int val);
    int multScale(int val);
    int divScale(int val);

};

#endif // PARAMEDITINT_H
