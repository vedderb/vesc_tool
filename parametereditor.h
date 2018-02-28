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

#ifndef PARAMETEREDITOR_H
#define PARAMETEREDITOR_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include "configparams.h"

namespace Ui {
class ParameterEditor;
}

class ParameterEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit ParameterEditor(QWidget *parent = 0);
    ~ParameterEditor();
    void setParams(const ConfigParams *params);

private slots:
    void timerSlot();

    void on_paramRemoveButton_clicked();
    void on_paramDownButton_clicked();
    void on_paramUpButton_clicked();
    void on_paramOpenButton_clicked();
    void on_paramSaveButton_clicked();
    void on_paramResetButton_clicked();
    void on_serRemoveButton_clicked();
    void on_serDownButton_clicked();
    void on_serUpButton_clicked();
    void on_serAddButton_clicked();
    void on_paramList_doubleClicked(const QModelIndex &index);
    void on_enumAddButton_clicked();
    void on_enumRemoveButton_clicked();
    void on_enumMoveUpButton_clicked();
    void on_enumMoveDownButton_clicked();
    void on_actionLoad_XML_triggered();
    void on_actionSave_XML_as_triggered();
    void on_actionDeleteAll_triggered();
    void on_doubleTxTypeBox_currentIndexChanged(int index);
    void on_actionCalculatePacketSize_triggered();

private:
    Ui::ParameterEditor *ui;
    ConfigParams mParams;
    QTimer *mTimer;
    QLabel *mStatusLabel;
    int mStatusInfoTime;

    void updateUi();
    void setEditorValues(QString name, ConfigParam p);
    QString getEditorValues(ConfigParam *p);
    void addEnum(QString name);
    void showStatusInfo(QString info, bool isGood);

};

#endif // PARAMETEREDITOR_H
