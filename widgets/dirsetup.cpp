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

#include "dirsetup.h"
#include "ui_dirsetup.h"

#include "utility.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QFrame>

DirSetup::DirSetup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DirSetup)
{
    ui->setupUi(this);
    mVesc = 0;
}

DirSetup::~DirSetup()
{
    delete ui;
}

VescInterface *DirSetup::vesc() const
{
    return mVesc;
}

void DirSetup::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

void DirSetup::scanVescs()
{
    if (mVesc) {
        auto *wOld = ui->vescArea->widget();
        if (wOld) {
            wOld->deleteLater();
        }

        if (!mVesc->isPortConnected()) {
            mVesc->emitMessageDialog("Direction Setup",
                                     "You are not connected to the VESC. Connect in order to run direction setup.",
                                     false, false);
            return;
        }

        ui->refreshButton->setEnabled(false);
        ui->progressBar->setRange(0, 0);

        auto addViewer = [this](QString name, int dev) {
            auto *border = new QFrame;
            auto *l = new QHBoxLayout;

            auto *imageLabel = new QLabel;
            imageLabel->setPixmap(QPixmap(dev >= 0 ? "://res/icons/can_off.png" : "://res/icons/Connected-96.png"));
            imageLabel->setMaximumSize(50, 50);
            imageLabel->setScaledContents(true);
            l->addWidget(imageLabel);
            auto *nameLabel = new QLabel;
            nameLabel->setText(name);
            l->addWidget(nameLabel);
            auto *invBox = new QCheckBox;
            invBox->setChecked(Utility::getInvertDirection(mVesc, dev));
            invBox->setText("Inverted");
            l->addStretch();
            l->addWidget(invBox);

            auto *lc = new QVBoxLayout;
            auto *fwdBt = new QPushButton;
            fwdBt->setText("Fwd");
            lc->addWidget(fwdBt);
            auto *revBt = new QPushButton;
            revBt->setText("Rev");
            lc->addWidget(revBt);
            auto *btW = new QWidget;
            btW->setLayout(lc);
            l->addWidget(btW);

            connect(fwdBt, &QPushButton::clicked, [this,dev]() {
                setEnabled(false);
                ui->progressBar->setRange(0, 0);
                Utility::testDirection(mVesc, dev, 0.1, 2000);
                ui->progressBar->setRange(0, 100);
                ui->progressBar->setValue(100);
                setEnabled(true);
            });

            connect(revBt, &QPushButton::clicked, [this,dev]() {
                setEnabled(false);
                ui->progressBar->setRange(0, 0);
                Utility::testDirection(mVesc, dev, -0.1, 2000);
                ui->progressBar->setRange(0, 100);
                ui->progressBar->setValue(100);
                setEnabled(true);
            });

            connect(invBox, &QCheckBox::toggled, [this,dev](bool checked) {
                setEnabled(false);
                ui->progressBar->setRange(0, 0);
                Utility::setInvertDirection(mVesc, dev, checked);
                ui->progressBar->setRange(0, 100);
                ui->progressBar->setValue(100);
                setEnabled(true);
            });

            border->setLayout(l);
            border->setFrameShape(QFrame::Box);
            border->setLineWidth(2);
            return border;
        };

        QVBoxLayout *l = new QVBoxLayout;
        l->setSpacing(4);
        l->addWidget(addViewer(QString("Local VESC"), -1));

        auto canDevs = mVesc->scanCan();
        for (auto d: canDevs) {
            l->addWidget(addViewer(QString("CAN VESC\nID: %1").arg(d), d));
        }

        l->addStretch();
        auto *w = new QWidget;
        w->setLayout(l);
        ui->vescArea->setWidget(w);

        ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(100);
        ui->refreshButton->setEnabled(true);
    }
}

void DirSetup::on_refreshButton_clicked()
{
    scanVescs();
}
