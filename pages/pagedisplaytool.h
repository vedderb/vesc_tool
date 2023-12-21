/*
    Copyright 2023 Benjamin Vedder	benjamin@vedder.se

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

#ifndef PAGEDISPLAYTOOL_H
#define PAGEDISPLAYTOOL_H

#include <QWidget>

namespace Ui {
class PageDisplayTool;
}

class PageDisplayTool : public QWidget
{
    Q_OBJECT

public:
    explicit PageDisplayTool(QWidget *parent = nullptr);
    ~PageDisplayTool();

private slots:
    void updateOverlay();
    void on_exportFontButton_clicked();
    void on_ovSaveButton_clicked();
    void on_updateSizeButton_clicked();
    void on_updateSizeButtonDisp_clicked();
    void on_fontEditExportButton_clicked();
    void on_fontEditApplyButton_clicked();
    void on_fontEditImportButton_clicked();

private:
    QFont getSelectedFont(bool antialias);
    QFont getSelectedFontEditor();
    void reloadFontEditor();
    void reloadFontEditorOverlay();

    Ui::PageDisplayTool *ui;
    bool mOverlayUpdating;

};

#endif // PAGEDISPLAYTOOL_H
