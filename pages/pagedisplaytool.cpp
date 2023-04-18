/*
    Copyright 2019 - 2023 Benjamin Vedder	benjamin@vedder.se

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

#include "pagedisplaytool.h"
#include "ui_pagedisplaytool.h"

#include <QLayout>
#include <QFileDialog>

PageDisplayTool::PageDisplayTool(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageDisplayTool)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);

    mOverlayUpdating = false;
    on_updateSizeButton_clicked();

    connect(ui->ovCrHBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovCrWBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovCrXPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovCrYPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->overlayBox, SIGNAL(toggled(bool)), this, SLOT(updateOverlay()));
    connect(ui->ovImCXPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovImCYPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovRotBox, SIGNAL(valueChanged(double)), this, SLOT(updateOverlay()));
    connect(ui->ovScaleBox, SIGNAL(valueChanged(double)), this, SLOT(updateOverlay()));
    connect(ui->ovRXPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovRYPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovXPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovYPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->ovTrBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));

    connect(ui->fontOverlayBox, SIGNAL(toggled(bool)), this, SLOT(updateOverlay()));
    connect(ui->fontBoldBox, SIGNAL(toggled(bool)), this, SLOT(updateOverlay()));
    connect(ui->fontBorderBox, SIGNAL(toggled(bool)), this, SLOT(updateOverlay()));
    connect(ui->fontAABox, SIGNAL(toggled(bool)), this, SLOT(updateOverlay()));
    connect(ui->fontXPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->fontYPosBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->fontWBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->fontHBox, SIGNAL(valueChanged(int)), this, SLOT(updateOverlay()));
    connect(ui->fontBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(updateOverlay()));
    connect(ui->fontSampleEdit, SIGNAL(textChanged(QString)), this, SLOT(updateOverlay()));
    connect(ui->fontScaleBox, SIGNAL(valueChanged(double)), this, SLOT(updateOverlay()));

    updateOverlay();

    connect(ui->fullEditor->getEdit(), &DisplayEdit::imageUpdated, [this](QImage image) {
        (void)image;

        if (!mOverlayUpdating) {
            updateOverlay();
        }
    });
}

PageDisplayTool::~PageDisplayTool()
{
    delete ui;
}

void PageDisplayTool::updateOverlay()
{
    mOverlayUpdating = true;

    ui->fullEditor->getEdit()->clearOverlayImage();

    if (ui->overlayBox->isChecked()) {
        QColor transparent = ui->fullEditor->paletteColor(ui->ovTrBox->value());

        if (ui->ovTrBox->value() < 0) {
            transparent = Qt::red;
        }

        ui->fullEditor->getEdit()->setOverlayImage(
                    ui->ovXPosBox->value(), ui->ovYPosBox->value(),
                    ui->ovCrXPosBox->value(), ui->ovCrWBox->value(),
                    ui->ovCrYPosBox->value(), ui->ovCrHBox->value(),
                    ui->ovImCXPosBox->value(), ui->ovImCYPosBox->value(),
                    ui->ovRXPosBox->value(), ui->ovRYPosBox->value(),
                    ui->ovRotBox->value(),
                    ui->ovScaleBox->value(),
                    transparent,
                    ui->customEditor->getEdit()->getImageNow());
    }

    QString str = ui->fontSampleEdit->text();
    if (ui->fontOverlayBox->isChecked() && str.size() > 0) {
        int w = ui->fontWBox->value();
        int h = ui->fontHBox->value();
        int xPos = ui->fontXPosBox->value();
        int yPos = ui->fontYPosBox->value();
        QImage img(w * str.size(), h, QImage::Format_ARGB32);
        QPainter p(&img);

        p.drawImage(img.rect(), ui->fullEditor->getEdit()->getImageNow(),
                    QRect(xPos, yPos, w * str.size(), h));

        p.setFont(getSelectedFont(ui->fontAABox->isChecked()));

        for (int i = 0;i < str.size();i++) {
            if (ui->fontBorderBox->isChecked()) {
                p.setPen(Qt::darkRed);
                p.drawRect(i * w, 0, w - 1, h - 1);
            }
            p.setPen(Qt::white);
            p.drawText(QRect(i * w, 0, w, h), Qt::AlignCenter, str.at(i));
        }

        for (int i = 0;i < img.width();i++) {
            for (int j = 0;j < img.height();j++) {
                QColor c = img.pixelColor(i, j);
                if (c != Qt::darkRed) {
                    int gray = qGray(c.rgb());
                    gray /= 16;
                    img.setPixelColor(i, j, ui->fullEditor->paletteColor(gray));
                }
            }
        }

        ui->fullEditor->getEdit()->setOverlayImage(
                    xPos, yPos,
                    0, 512, 0, 512, 0, 0, 0, 0, 0, 1.0, Qt::black,
                    img);
    }

    mOverlayUpdating = false;
}

void PageDisplayTool::on_exportFontButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Binary File"), "",
                                                    tr("Bin files (*.bin *.BIN *.Bin)"));

    if (filename.isEmpty()) {
        return;
    }

    if (!filename.endsWith(".bin", Qt::CaseInsensitive)) {
        filename.append(".bin");
    }

    QFile f(filename);

    if (!f.open(QIODevice::WriteOnly)) {
        return;
    }

    int w = ui->fontWBox->value();
    int h = ui->fontHBox->value();
    int bytesPerChar = (w * h) / 8;
    int charNum = ui->exportCustomNumOnlyBox->isChecked() ? 10 : 95;

    if ((w * h) % 8 != 0) {
        bytesPerChar++;
    }

    QByteArray fontArr;
    fontArr.resize(bytesPerChar * charNum + 4);

    for (auto &c: fontArr) {
        c = 0;
    }

    fontArr[0] = w;
    fontArr[1] = h;
    fontArr[2] = charNum;
    fontArr[3] = 1; // 1 bit per char

    for (int ch = 0;ch < charNum;ch++) {
        QImage img(w, h, QImage::Format_ARGB32);
        QPainter p(&img);

        p.fillRect(img.rect(), Qt::black);
        p.setPen(Qt::white);

        p.setFont(getSelectedFont(false));
        p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, QChar(ch + (charNum == 10 ? '0' : ' ')));

        for (int i = 0;i < w * h;i++) {
            QColor px = img.pixel(i % w, i / w);
            char c = fontArr[4 + bytesPerChar * ch + i / 8];
            c |= (px != Qt::black) << (i % 8);
            fontArr[4 + bytesPerChar * ch + (i / 8)] = c;
        }
    }

    f.write(fontArr);
    f.close();
}


void PageDisplayTool::on_ovSaveButton_clicked()
{
    ui->fullEditor->getEdit()->saveOverlayToLayer2();
}


void PageDisplayTool::on_updateSizeButton_clicked()
{
    ui->customEditor->updateSize(ui->wBox->value(), ui->hBox->value());
}

QFont PageDisplayTool::getSelectedFont(bool antialias)
{
    QFont f = ui->fontBox->currentFont();
    f.setPointSizeF(500.0);
    f.setBold(ui->fontBoldBox->isChecked());

    if (!antialias) {
        f.setStyleStrategy(QFont::NoAntialias);
    }

    QFontMetrics fm(f);
    double fh = (500.0 * (double)ui->fontHBox->value()) / (double)fm.ascent();
    f.setPointSizeF(fh * ui->fontScaleBox->value());
    return f;
}


void PageDisplayTool::on_updateSizeButtonDisp_clicked()
{
    ui->fullEditor->updateSize(ui->wBoxDisp->value(), ui->hBoxDisp->value());
}

