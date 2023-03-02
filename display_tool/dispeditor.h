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

#ifndef DISPEDITOR_H
#define DISPEDITOR_H

#include <QWidget>
#include "displayedit.h"

namespace Ui {
class DispEditor;
}

class DispEditor : public QWidget
{
    Q_OBJECT

public:
    explicit DispEditor(QWidget *parent = 0);
    ~DispEditor();

    void updateSize(int width, int height);
    DisplayEdit *getEdit();
    QColor paletteColor(int ind);

private slots:
    void on_saveCButton_clicked();
    void on_loadCButton_clicked();
    void on_savePngButton_clicked();
    void on_loadPngButton_clicked();
    void on_showLayer2Box_toggled(bool checked);
    void on_clearLayer2Button_clicked();
    void on_clearButton_clicked();
    void on_imgScaleBox_valueChanged(int arg1);
    void on_formatBox_currentIndexChanged(int index);
    void on_helpButton_clicked();

private:
    Ui::DispEditor *ui;
    QVector<QColor> mPalette;
    void updatePalette();
    int imgBits();

};

#endif // DISPEDITOR_H
