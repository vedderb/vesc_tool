/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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


#include "pageloganalysis.h"
#include "ui_pageloganalysis.h"

PageLogAnalysis::PageLogAnalysis(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageLogAnalysis)
{
    ui->setupUi(this);
    mVesc = nullptr;
}

PageLogAnalysis::~PageLogAnalysis()
{
    delete ui;
}

VescInterface *PageLogAnalysis::vesc() const
{
    return mVesc;
}

void PageLogAnalysis::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

void PageLogAnalysis::on_openCsvButton_clicked()
{
    if (mVesc) {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Load CSV File"), "",
                                                        tr("CSV files (*.csv)"));

        if (!fileName.isEmpty()) {
            if (mVesc->loadRtLogFile(fileName)) {
                on_openCurrentButton_clicked();
            }
        }
    }
}

void PageLogAnalysis::on_openCurrentButton_clicked()
{
    if (mVesc) {
        auto data = mVesc->getRtLogData();

        ui->map->clearAllInfoTraces();
        for (auto d: data) {
            if (d.posTime >= 0) {

            }
        }
    }
}

void PageLogAnalysis::on_gridBox_toggled(bool checked)
{
    ui->map->setDrawGrid(checked);
}

void PageLogAnalysis::on_tilesHiResButton_toggled(bool checked)
{
    if (checked) {
        ui->map->osmClient()->setTileServerUrl("http://c.osm.rrze.fau.de/osmhd");
        ui->map->osmClient()->clearCache();
        ui->map->update();
    }
}

void PageLogAnalysis::on_tilesOsmButton_toggled(bool checked)
{
    if (checked) {
        ui->map->osmClient()->setTileServerUrl("http://tile.openstreetmap.org");
        ui->map->osmClient()->clearCache();
        ui->map->update();
    }
}
