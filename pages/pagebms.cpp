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
#include "ui_pagebms.h"
#include "utility.h"

PageBms::PageBms(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageBms)
{
    ui->setupUi(this);
    mVesc = nullptr;

    ui->balOnButton->setIcon(Utility::getIcon("/icons/Circled Play-96.png"));
    ui->chgEnButton->setIcon(Utility::getIcon("/icons/Circled Play-96.png"));
    ui->balOffButton->setIcon(Utility::getIcon("/icons/Stop-96.png"));
    ui->chgDisButton->setIcon(Utility::getIcon("/icons/Stop-96.png"));
    ui->resetAhButton->setIcon(Utility::getIcon("/icons/Restart-96.png"));
    ui->resetWhButton->setIcon(Utility::getIcon("/icons/Restart-96.png"));
    ui->zeroCurrentButton->setIcon(Utility::getIcon("/icons/Refresh-96.png"));

    ui->valTable->setColumnWidth(0, 200);
    ui->splitter->setSizes(QList<int>({1000, 500}));
    Utility::setPlotColors(ui->plotCells);
    Utility::setPlotColors(ui->plotTemp);
}

PageBms::~PageBms()
{
    delete ui;
}

VescInterface *PageBms::vesc() const
{
    return mVesc;
}

void PageBms::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(bmsValuesRx(BMS_VALUES)),
                this, SLOT(bmsValuesRx(BMS_VALUES)));
    }
}

void PageBms::bmsValuesRx(BMS_VALUES val)
{
    if (mCellBars.size() != val.v_cells.size()) {
        reloadCellBars(val.v_cells.size());
    }

    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    for (int i = 0;i < val.v_cells.size();i++) {
        QVector<double> cell, voltage;
        cell.append(i + 1);
        voltage.append(val.v_cells.at(i));
        mCellBars.at(i)->setData(cell, voltage);
        mCellBars.at(i)->setBrush(val.is_balancing.at(i) ?
                                      QColor(255, 99, 71) : QColor(10, 140, 70, 160));

        textTicker->addTick(i + 1, QString("C%1 (%2 V)").
                            arg(i + 1).arg(val.v_cells.at(i)));
    }

    ui->plotCells->xAxis->setTicker(textTicker);
    ui->plotCells->replotWhenVisible();

    // Temps

    if (mVesc->getLastFwRxParams().hw == "12s7p") {
        ui->tempPlot->load12s7p();
        ui->tempPlot->setValues(val);
    } else {
        ui->tempPlot->unload();
    }

    if (mTempBars.size() != (val.temps.size() + 1)) {
        reloadTempBars(val.temps.size() + 1);
    }

    QSharedPointer<QCPAxisTickerText> textTicker2(new QCPAxisTickerText);
    val.temps.prepend(val.temp_ic);
    for (int i = 0;i < val.temps.size();i++) {
        double t = val.temps.at(i);
        QVector<double> sensor, temp;
        sensor.append(i + 1);
        temp.append(t);
        mTempBars.at(i)->setData(sensor, temp);
        mTempBars.at(i)->setBrush(t > 55 ? Utility::getAppQColor("red") : Utility::getAppQColor("blue"));

        textTicker2->addTick(i + 1, QString("T%1 (%2 °C)").
                            arg(i + 1).arg(t));
    }

    ui->plotTemp->xAxis->setTicker(textTicker2);
    ui->plotTemp->replotWhenVisible();

    // Value table
    ui->valTable->item(0, 0)->setText(QString("%1 V").arg(val.v_tot, 0, 'f', 2));
    ui->valTable->item(1, 0)->setText(QString("%1 V").arg(val.v_charge, 0, 'f', 2));
    ui->valTable->item(2, 0)->setText(QString("%1 A").arg(val.i_in, 0, 'f', 2));
    ui->valTable->item(3, 0)->setText(QString("%1 A").arg(val.i_in_ic, 0, 'f', 2));
    ui->valTable->item(4, 0)->setText(QString("%1 Ah").arg(val.ah_cnt, 0, 'f', 3));
    ui->valTable->item(5, 0)->setText(QString("%1 Wh").arg(val.wh_cnt, 0, 'f', 3));
    ui->valTable->item(6, 0)->setText(QString("%1 W").arg(val.i_in_ic * val.v_tot, 0, 'f', 3));
    ui->valTable->item(7, 0)->setText(QString("%1 %").arg(val.soc * 100.0, 0, 'f', 0));
    ui->valTable->item(8, 0)->setText(QString("%1 %").arg(val.soh * 100.0, 0, 'f', 0));
    ui->valTable->item(9, 0)->setText(QString("%1 °C").arg(val.temp_cells_highest, 0, 'f', 2));
    ui->valTable->item(10, 0)->setText(QString("%1 %").arg(val.humidity, 0, 'f', 2));
    ui->valTable->item(11, 0)->setText(QString("%1 Pa").arg(val.pressure, 0, 'f', 0));
    ui->valTable->item(12, 0)->setText(QString("%1 °C").arg(val.temp_hum_sensor, 0, 'f', 2));
    ui->valTable->item(13, 0)->setText(QString("%1 Ah").arg(val.ah_cnt_chg_total, 0, 'f', 3));
    ui->valTable->item(14, 0)->setText(QString("%1 Wh").arg(val.wh_cnt_chg_total, 0, 'f', 3));
    ui->valTable->item(15, 0)->setText(QString("%1 Ah").arg(val.ah_cnt_dis_total, 0, 'f', 3));
    ui->valTable->item(16, 0)->setText(QString("%1 Wh").arg(val.wh_cnt_dis_total, 0, 'f', 3));
}

void PageBms::reloadCellBars(int cells)
{
    mCellBars.clear();
    ui->plotCells->clearItems();
    ui->plotCells->clearGraphs();
    ui->plotCells->clearPlottables();

    for (int i = 0;i < cells;i++) {
        mCellBars.append(new QCPBars(ui->plotCells->xAxis, ui->plotCells->yAxis));
        mCellBars.last()->setPen(Qt::NoPen);
        mCellBars.last()->setBrush(QColor(10, 140, 70, 160));
    }

    ui->plotCells->xAxis->setRange(0.0, cells + 1);
    ui->plotCells->yAxis->setRange(2.5, 4.4);
    ui->plotCells->yAxis->setLabel("Voltage");

    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    for (int i = 0;i < 12;i++) {
        textTicker->addTick(i + 1, QString("C%1").arg(i + 1));
    }
    ui->plotCells->xAxis->setTicker(textTicker);
    ui->plotCells->xAxis->setTickLabelRotation(60);
    ui->plotCells->xAxis->setSubTicks(false);
    ui->plotCells->xAxis->setTickLength(0, 1);

    ui->plotCells->xAxis->grid()->setVisible(true);
    ui->plotCells->xAxis->grid()->setPen(QPen(QColor(130, 130, 130), 0, Qt::DotLine));
}

void PageBms::reloadTempBars(int sensors)
{
    mTempBars.clear();
    ui->plotTemp->clearItems();
    ui->plotTemp->clearGraphs();
    ui->plotTemp->clearPlottables();

    for (int i = 0;i < sensors;i++) {
        mTempBars.append(new QCPBars(ui->plotTemp->xAxis, ui->plotTemp->yAxis));
        mTempBars.last()->setPen(Qt::NoPen);
        mTempBars.last()->setBrush(Qt::darkBlue);
    }

    ui->plotTemp->xAxis->setRange(0.0, sensors + 1);
    ui->plotTemp->yAxis->setRange(-20.0, 90.0);
    ui->plotTemp->yAxis->setLabel("Temperature (degC)");

    QSharedPointer<QCPAxisTickerText> textTickerTemp(new QCPAxisTickerText);

    for (int i = 0;i < sensors;i++) {
        textTickerTemp->addTick(i + 1, QString("S%1").arg(i));
    }
    ui->plotTemp->xAxis->setTicker(textTickerTemp);
    ui->plotTemp->xAxis->setTickLabelRotation(60);
    ui->plotTemp->xAxis->setSubTicks(false);
    ui->plotTemp->xAxis->setTickLength(0, 1);
}

void PageBms::on_resetAhButton_clicked()
{
    mVesc->commands()->bmsResetCounters(true, false);
}

void PageBms::on_resetWhButton_clicked()
{
    mVesc->commands()->bmsResetCounters(false, true);
}

void PageBms::on_balOnButton_clicked()
{
    mVesc->commands()->bmsForceBalance(true);
}

void PageBms::on_balOffButton_clicked()
{
    mVesc->commands()->bmsForceBalance(false);
}

void PageBms::on_zeroCurrentButton_clicked()
{
    mVesc->commands()->bmsZeroCurrentOffset();
}

void PageBms::on_chgEnButton_clicked()
{
    mVesc->commands()->bmsSetChargeAllowed(true);
}

void PageBms::on_chgDisButton_clicked()
{
    mVesc->commands()->bmsSetChargeAllowed(false);
}
