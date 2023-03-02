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

#include "dispeditor.h"
#include "ui_dispeditor.h"
#include "utility.h"

#include <QPushButton>
#include <QFileDialog>
#include <QDebug>
#include <QColor>
#include <QImage>
#include <QColorDialog>

DispEditor::DispEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DispEditor)
{
    ui->setupUi(this);
    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));

    updateSize(128, 128);

    ui->imageWidget->setPixmap(QPixmap::fromImage(ui->displayEdit->getImageNow()));

    connect(ui->displayEdit, &DisplayEdit::imageUpdated, [this](QImage image) {
        ui->imageWidget->setPixmap(QPixmap::fromImage(image));
        ui->imageWidget->update();
    });

    connect(ui->displayEdit, &DisplayEdit::editColorChanged, [this](QColor c) {
        QPixmap px(50, 50);
        QPainter painter(&px);
        painter.fillRect(0, 0, px.width(), px.height(), c);
        ui->ColorNowLabel->setPixmap(px);
    });

    updatePalette();
    on_imgScaleBox_valueChanged(ui->imgScaleBox->value());
}

void DispEditor::updateSize(int width, int height)
{
    ui->displayEdit->setImageSize(width, height);
    ui->imageWidget->setMinimumWidth(width * ui->imgScaleBox->value());
    ui->imageWidget->setMaximumWidth(width * ui->imgScaleBox->value());
    ui->imageWidget->setMinimumHeight(height * ui->imgScaleBox->value());
    ui->imageWidget->setMaximumHeight(height * ui->imgScaleBox->value());
    update();
}

DisplayEdit *DispEditor::getEdit()
{
    return ui->displayEdit;
}

QColor DispEditor::paletteColor(int ind)
{
    if (ind >= 0 && ind < mPalette.size()) {
        return mPalette.at(ind);
    } else {
        return Qt::white;
    }
}

DispEditor::~DispEditor()
{
    delete ui;
}

void DispEditor::on_saveCButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Binary Image"), "",
                                                    tr("Bin files (*.bin *.BIN *.Bin)"));

    if (!filename.isEmpty()) {
        if (!filename.endsWith(".bin", Qt::CaseInsensitive)) {
            filename.append(".bin");
        }

        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly)) {
            return;
        }

        QImage img = ui->displayEdit->getImageNow();
        int w = img.width();
        int h = img.height();

        VByteArray imgArr;
        int colors = mPalette.size();
        int bits = 0;
        while (colors >>= 1) {
            bits++;
        }

        for (auto &c: imgArr) {
            c = 0;
        }

        imgArr.vbAppendInt16(w);
        imgArr.vbAppendInt16(h);
        imgArr.vbAppendInt8(bits);

        int arrOfs = 0;
        int bitCnt = 0;
        uint8_t pixNow = 0;
        for (int j = 0;j < img.height();j++) {
            for (int i = 0;i < img.width();i++) {
                auto pix = mPalette.indexOf(img.pixelColor(i, j));
                pixNow >>= bits;
                pixNow |= pix << (8 - bits);
                bitCnt += bits;

                if (bitCnt >= 8) {
                    imgArr.vbAppendUint8(pixNow);
                    pixNow = 0;
                    bitCnt = 0;
                }
            }
        }

        f.write(imgArr);
        f.close();
    }
}

void DispEditor::on_loadCButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Load Binary Image"), "",
                                                    tr("Bin files (*.bin *.BIN *.Bin)"));

    if (!filename.isEmpty()) {
        QFile f(filename);
        if (!f.open(QIODevice::ReadOnly)) {
            return;
        }
        VByteArray data = f.readAll();
        f.close();

        int w = data.vbPopFrontInt16();
        int h = data.vbPopFrontInt16();
        int bits = data.vbPopFrontUint8();

        if (((w * h * bits) / 8) != data.size()) {
            qWarning() << "Invalid binary image file";
            return;
        }

        updateSize(w, h);
        QImage img(w, h, QImage::Format_ARGB32);
        img.fill(Qt::black);
        uint8_t pixNow = data.vbPopFrontUint8();
        int bitCnt = 0;

        for (int j = 0;j < h;j++) {
            for (int i = 0;i < w;i++) {
                int pix = pixNow & ~(0xff << bits);
                pixNow >>= bits;
                bitCnt += bits;
                if (bitCnt >= 8) {
                    pixNow = data.vbPopFrontUint8();
                    bitCnt = 0;
                }

                if (pix < mPalette.size()) {
                    img.setPixelColor(i, j, mPalette.at(pix));
                }
            }
        }

        ui->displayEdit->loadFromImage(img);
    }
}

void DispEditor::on_savePngButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save PNG File"), "",
                                                    tr("PNG files (*.png *.Png *.PNG)"));

    if (!filename.isEmpty()) {
        if (!filename.toLower().endsWith(".png")) {
            filename.append(".png");
        }

        ui->displayEdit->getImageNow().save(filename, "PNG");
    }
}

void DispEditor::on_loadPngButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Load Image File"), "",
                                                    tr("JPG and PNG files (*.png *.Png *.PNG *.jpg *.Jpg *.JPG *.jpeg *.Jpeg *.JPEG)"));

    if (!filename.isEmpty()) {
        QImage img;
        img.load(filename);
        QImage img2(img.size(), QImage::Format_ARGB32);

        QPainter p(&img2);
        p.drawImage(0, 0, img);

        img = img2.scaled(ui->displayEdit->getImageSize(),
                         Qt::KeepAspectRatioByExpanding,
                         ui->antialiasBox->isChecked() ?
                             Qt::SmoothTransformation :
                             Qt::FastTransformation);

        bool validPalette = true;
        for (int i = 0;i < img.width();i++) {
            for (int j = 0;j < img.height();j++) {
                if (!mPalette.contains(img.pixelColor(i, j))) {
                    validPalette = false;
                    break;
                }
            }
        }

        if (!validPalette) {
            if (ui->ditherBox->isChecked()) {
                int imgW = img.width();
                int imgH = img.height();

                int bits = 4;
                if (mPalette.size() == 2) {
                    bits = 1;
                } else if (mPalette.size() == 4) {
                    bits = 2;
                }

                int **img_buffer = new int*[imgW];
                for(int i = 0; i < imgW; i++) {
                    img_buffer[i] = new int[imgH];
                }

                int mask = (0xFF << (8 - bits)) & 0xFF;

                for (int y = 0;y < imgH;y++) {
                    for (int x = 0;x < imgW;x++) {
                        QRgb col = img.pixel(x, y);
                        img_buffer[x][y] = qGray(col);
                    }
                }

                for (int y = 0;y < imgH;y++) {
                    for (int x = 0;x < imgW;x++) {
                        int pix = img_buffer[x][y];

                        if (pix > 255) {
                            pix = 255;
                        }

                        if (pix < 0) {
                            pix = 0;
                        }

                        int pix_n = pix & mask;
                        int quant_error = pix - pix_n;
                        img_buffer[x][y]= pix_n;

                        if (x < (imgW - 1)) {
                            img_buffer[x + 1][y] += ((quant_error << 12) * 7) >> 16;
                        }

                        if (y < (imgH - 1)) {
                            img_buffer[x][y + 1] += ((quant_error << 12) * 5) >> 16;
                        }

                        if (x > 0 && y < (imgH - 1)) {
                            img_buffer[x - 1][y + 1] += ((quant_error << 12) * 3) >> 16;
                        }

                        if (x < (imgW - 1) && y < (imgH - 1)) {
                            img_buffer[x + 1][y + 1] += ((quant_error << 12) * 1) >> 16;
                        }
                    }
                }

                for (int y = 0;y < imgH;y++) {
                    for (int x = 0;x < imgW;x++) {
                        img.setPixelColor(x, y, mPalette.at(img_buffer[x][y] / (256 / mPalette.size())));
                    }
                }

                for(int i = 0; i < imgW; i++) {
                    delete[] img_buffer[i];
                }
                delete[] img_buffer;
            } else {
                for (int i = 0;i < img.width();i++) {
                    for (int j = 0;j < img.height();j++) {
                        QColor c = img.pixelColor(i, j);
                        int gray = qGray(c.rgb());
                        gray /= (256 / mPalette.size());

                        if (256 % mPalette.size() != 0) {
                            gray += 1;
                        }

                        img.setPixelColor(i, j, mPalette.at(gray));
                    }
                }
            }
        }

        ui->displayEdit->loadFromImage(img);
    }
}

void DispEditor::on_showLayer2Box_toggled(bool checked)
{
    ui->displayEdit->setDrawLayer2(checked);
}

void DispEditor::on_clearLayer2Button_clicked()
{
    ui->displayEdit->clearLayer2();
}

void DispEditor::on_clearButton_clicked()
{
    ui->displayEdit->clearImage();
}

void DispEditor::on_imgScaleBox_valueChanged(int arg1)
{
    int width = ui->displayEdit->getImageSize().width();
    int height = ui->displayEdit->getImageSize().height();
    ui->imageWidget->setMinimumWidth(width * arg1);
    ui->imageWidget->setMaximumWidth(width * arg1);
    ui->imageWidget->setMinimumHeight(height * arg1);
    ui->imageWidget->setMaximumHeight(height * arg1);
}

void DispEditor::updatePalette()
{
    QGridLayout *l = new QGridLayout;
    l->setVerticalSpacing(1);
    l->setHorizontalSpacing(1);
    l->setMargin(1);

    int colors = 16;
    if (ui->formatBox->currentIndex() == 0) {
        colors = 2;
    } else if (ui->formatBox->currentIndex() == 1) {
        colors = 4;
    }

    mPalette.clear();

    for (int i = 0;i < colors;i++) {
        QColor c = QColor::fromRgbF((double)i / ((double)colors - 1.0),
                                    (double)i / ((double)colors - 1.0),
                                    (double)i / ((double)colors - 1.0));

        QPushButton *b = new QPushButton();
        auto updateColor = [b, this, i] (QColor col) {
            QPixmap px(50, 50);
            QPainter painter(&px);
            painter.fillRect(0, 0, px.width(), px.height(), col);
            b->setIcon(px);
            mPalette[i] = col;
            ui->ColorNowLabel->setPixmap(px);
            ui->displayEdit->setEditColor(col);
        };

        const int cols = 6;
        l->addWidget(b, i / cols, i - cols * (i / cols));
        mPalette.append(c);
        updateColor(c);

        if (i == 15) {
            ui->displayEdit->setEditColor(c);
        }

        connect(b, &QPushButton::clicked, [updateColor,b,i,this](bool) {
            auto mod = QGuiApplication::keyboardModifiers();
            if (mod == Qt::ControlModifier) {
                auto color = QColorDialog::getColor(mPalette.at(i), b, "Choose Color", QColorDialog::DontUseNativeDialog);
                updateColor(color);
            } else {
                updateColor(mPalette.at(i));
            }
        });
    }

    auto layoutOld = ui->colorWidget->layout();
    if (layoutOld != nullptr) {
        QLayoutItem *child;
        while ((child = layoutOld->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete layoutOld;
    }

    ui->colorWidget->setLayout(l);
}

void DispEditor::on_formatBox_currentIndexChanged(int index)
{
    (void)index;
    updatePalette();
}

void DispEditor::on_helpButton_clicked()
{
    QMessageBox::information(this, "Usage Instructions",
                             "<b>Navigate in editor</b><br>"
                             "Left-click and drag to move. Scroll to zoom.<br>"
                             "<br>"
                             "<b>Draw pixels</b><br>"
                             "Shift + Left-click (and drag)<br>"
                             "<br>"
                             "<b>Change color</b><br>"
                             "Click on color buttons, or shift + right-click on pixel with desired color.<br>"
                             "<br>"
                             "<b>Update Palette Color</b><br>"
                             "Ctrl + left-click on the palette buttons.<br>"
                             "<br>"
                             "<b>Overlay</b><br>"
                             "Overlay is an important function of this editor, which overlays an image "
                             "from the custom tab to the Full Frame tab with a transform. The interesting aspect "
                             "is that the same transforms are available on the controller, meaning that all transform "
                             "parameters can be animated. For example, if a rotating needle should show speed, "
                             "the needle can be drawn pointing to the right in the Custom tab and the rotation "
                             "can be tested here. The same rotation can be achieved on the remote. One can also "
                             "become creative with animating other properties of the overlay. To get information "
                             "about what the transform parameters do, the mouse can be hovered above the text to "
                             "the left of them.<br>"
                             "<br>"
                             "The custom tab supports images of any size, as long as the width and height is an "
                             "even number. The size is limited by the memory on the microcontroller."
                             "<br>"
                             "<b>Layer 2</b><br>"
                             "The second layer can be used to draw overlays on, without messing with the main image. "
                             "This way the background image can be kept clean for when it is saved.");
}
