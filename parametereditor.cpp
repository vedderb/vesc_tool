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

#include "parametereditor.h"
#include "ui_parametereditor.h"
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <cmath>

ParameterEditor::ParameterEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ParameterEditor)
{
    ui->setupUi(this);
    setEditorValues("new_parameter", ConfigParam());

    mStatusInfoTime = 0;
    mStatusLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(mStatusLabel);
    mTimer = new QTimer(this);
    mTimer->start(20);

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(timerSlot()));

    connect(ui->groupList, &QListWidget::currentRowChanged, [this](int row) {
        (void)row;
        updateSubgroupList();
    });

    connect(ui->subgroupList, &QListWidget::currentRowChanged, [this](int row) {
        (void)row;
        updateGroupParamList();
    });
}

ParameterEditor::~ParameterEditor()
{
    delete ui;
}

void ParameterEditor::setParams(const ConfigParams *params)
{
    QStringList paramList = params->getParamOrder();
    mParams.clearParams();
    mParams.setSerializeOrder(params->getSerializeOrder());
    mParams.setGrouping(params->getGrouping());

    for (int i = 0;i < paramList.size();i++) {
        QString name = paramList.at(i);
        mParams.addParam(name, params->getParamCopy(name));
    }

    mParams.setUpdatesEnabled(false);

    showStatusInfo(tr("Parameters Loaded"), true);

    updateUi();
}

void ParameterEditor::timerSlot()
{
    // Update status label
    if (mStatusInfoTime) {
        mStatusInfoTime--;
        if (!mStatusInfoTime) {
            mStatusLabel->setStyleSheet(qApp->styleSheet());
        }
    } else {
        if (!mStatusLabel->text().isEmpty()) {
            mStatusLabel->clear();
        }
    }
}

void ParameterEditor::on_paramRemoveButton_clicked()
{
    int row = ui->paramList->currentRow();

    if (row >= 0) {
        mParams.deleteParam(ui->paramList->item(row)->text());
        delete ui->paramList->takeItem(row);
    }
}

void ParameterEditor::on_paramDownButton_clicked()
{
    int row = ui->paramList->currentRow();
    QStringList order = mParams.getParamOrder();

    if (row >= 0 && row < (order.size() - 1)) {
        QString name = order.at(row);
        order.removeAt(row);
        order.insert(row + 1, name);
        mParams.setParamOrder(order);
        updateUi();
        ui->paramList->setCurrentRow(row + 1);
    }
}

void ParameterEditor::on_paramUpButton_clicked()
{
    int row = ui->paramList->currentRow();
    QStringList order = mParams.getParamOrder();

    if (row >= 1) {
        QString name = order.at(row);
        order.removeAt(row);
        order.insert(row - 1, name);
        mParams.setParamOrder(order);
        updateUi();
        ui->paramList->setCurrentRow(row - 1);
    }
}

void ParameterEditor::on_paramOpenButton_clicked()
{
    QListWidgetItem *item = ui->paramList->currentItem();
    QString selected;

    if (item) {
        selected = item->text();
    }

    ConfigParam *p = mParams.getParam(selected);

    if (p) {
        setEditorValues(selected, *p);

        if (ui->previewTable->rowCount() > 0) {
            ui->previewTable->removeRow(0);
        }

        if (p->type != CFG_T_UNDEFINED) {
            ui->previewTable->addParamRow(&mParams, selected);
        }
    }
}

void ParameterEditor::on_paramSaveButton_clicked()
{
    if (ui->nameEdit->text().contains(" ")) {
        QMessageBox::warning(this,
                             tr("Save Parameter"),
                             tr("Spaces are not allowed in the name."));
        return;
    }

    if (ui->nameEdit->text().isEmpty()) {
        QMessageBox::warning(this,
                             tr("Save Parameter"),
                             tr("Name is empty."));
        return;
    }

    QString name;
    ConfigParam p;

    name = getEditorValues(&p);

    if (mParams.hasParam(name)) {
        *mParams.getParam(name) = p;
        showStatusInfo(tr("Parameter updated: %1").arg(name), true);
    } else {
        mParams.addParam(name, p);

        int row = ui->paramList->currentRow();
        QStringList order = mParams.getParamOrder();
        order.removeLast();

        if (row < 0) {
            row = order.size();
        } else {
            row += 1;
        }

        order.insert(row, name);
        mParams.setParamOrder(order);

        updateUi();

        ui->paramList->setCurrentRow(row);
        showStatusInfo(tr("New parameter added: %1").arg(name), true);
    }

    if (ui->previewTable->rowCount() > 0) {
        ui->previewTable->removeRow(0);
    }

    if (p.type != CFG_T_QSTRING && p.type != CFG_T_UNDEFINED) {
        ui->previewTable->addParamRow(&mParams, name);
    }
}

void ParameterEditor::on_paramResetButton_clicked()
{
    setEditorValues("new_parameter", ConfigParam());
}

void ParameterEditor::on_serRemoveButton_clicked()
{
    int row = ui->paramSerialOrderList->currentRow();

    if (row >= 0) {
        QStringList order = mParams.getSerializeOrder();
        order.removeAt(row);
        mParams.setSerializeOrder(order);
        delete ui->paramSerialOrderList->takeItem(row);
    }
}

void ParameterEditor::on_serDownButton_clicked()
{
    int row = ui->paramSerialOrderList->currentRow();
    QStringList order = mParams.getSerializeOrder();

    if (row >= 0 && row < (order.size() - 1)) {
        QString name = order.at(row);
        order.removeAt(row);
        order.insert(row + 1, name);
        mParams.setSerializeOrder(order);
        updateUi();
        ui->paramSerialOrderList->setCurrentRow(row + 1);
    }
}

void ParameterEditor::on_serUpButton_clicked()
{
    int row = ui->paramSerialOrderList->currentRow();
    QStringList order = mParams.getSerializeOrder();

    if (row >= 1) {
        QString name = order.at(row);
        order.removeAt(row);
        order.insert(row - 1, name);
        mParams.setSerializeOrder(order);
        updateUi();
        ui->paramSerialOrderList->setCurrentRow(row - 1);
    }
}

void ParameterEditor::on_serAddButton_clicked()
{
    QListWidgetItem *item = ui->paramList->currentItem();
    QString selected = "";

    if (item) {
        selected = item->text();
    }

    bool ok;
    QString name = QInputDialog::getText(this,
                                         tr("Add to Serialization Order"),
                                         tr("Parameter Name"),
                                         QLineEdit::Normal,
                                         selected,
                                         &ok);

    if (ok) {
        if (name.contains(" ")) {
            QMessageBox::warning(this,
                                 tr("Add"),
                                 tr("Spaces are not allowed in the name."));
            return;
        }

        if (name.isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Add"),
                                 tr("Name is empty."));
            return;
        }

        int row = ui->paramSerialOrderList->currentRow();
        QStringList order = mParams.getSerializeOrder();

        if (row < 0 || row >= order.size()) {
            row = order.size() - 1;
        }

        order.insert(row + 1, name);
        mParams.setSerializeOrder(order);
        updateUi();
    }
}

void ParameterEditor::on_paramList_doubleClicked(const QModelIndex &index)
{
    (void)index;
    on_paramOpenButton_clicked();
}

void ParameterEditor::on_enumAddButton_clicked()
{
    addEnum(tr("<double-click to edit>"));
}

void ParameterEditor::on_enumRemoveButton_clicked()
{
    int row = ui->enumList->currentRow();

    if (row >= 0) {
        delete ui->enumList->takeItem(row);
    }
}

void ParameterEditor::on_enumMoveUpButton_clicked()
{
    int row = ui->enumList->currentRow();

    if (row >= 1) {
        QListWidgetItem *item = ui->enumList->takeItem(row);
        ui->enumList->insertItem(row - 1, item);
        ui->enumList->setCurrentRow(row - 1);
    }
}

void ParameterEditor::on_enumMoveDownButton_clicked()
{
    int row = ui->enumList->currentRow();

    if (row >= 0 && row < (ui->enumList->count() - 1)) {
        QListWidgetItem *item = ui->enumList->takeItem(row);
        ui->enumList->insertItem(row + 1, item);
        ui->enumList->setCurrentRow(row + 1);
    }
}

void ParameterEditor::on_actionLoad_XML_triggered()
{
    QString path;
    path = QFileDialog::getOpenFileName(this,
                                        tr("Choose parameter file to load"),
                                        ".",
                                        tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    bool res = mParams.loadParamsXml(path);

    if (!res) {
        QMessageBox::information(this,
                                 tr("Load Parameters"),
                                 tr("Could not load parameters:<BR>"
                                    "%1").arg(mParams.xmlStatus()));
    } else {
        showStatusInfo(tr("Configuration Loaded"), true);
    }

    updateUi();
}

void ParameterEditor::on_actionSave_XML_as_triggered()
{
    QString path;
    path = QFileDialog::getSaveFileName(this,
                                        tr("Choose where to save the parameter XML file"),
                                        ".",
                                        tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    if (!path.toLower().endsWith(".xml")) {
        path += ".xml";
    }

    bool res = mParams.saveParamsXml(path);

    if (!res) {
        QMessageBox::information(this,
                                 tr("Save Parameters"),
                                 tr("Could not save parameters:<BR>"
                                    "%1").arg(mParams.xmlStatus()));
    } else {
        showStatusInfo(tr("Configuration Saved"), true);
    }
}

void ParameterEditor::on_actionDeleteAll_triggered()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,
                                  tr("Delete All"),
                                  tr("Are you sure that you want to delete all data?"),
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        mParams.clearAll();
        updateUi();
        showStatusInfo(tr("Configuration Deleted"), true);
    }
}

void ParameterEditor::updateUi()
{
    ui->paramList->clear();
    ui->paramList->addItems(mParams.getParamOrder());
    ui->paramSerialOrderList->clear();
    ui->paramSerialOrderList->addItems(mParams.getSerializeOrder());
    updateGroupList();
}

void ParameterEditor::setEditorValues(QString name, ConfigParam p)
{
    ui->nameEdit->setText(name);
    ui->longNameEdit->setText(p.longName);
    ui->typeBox->setCurrentIndex(p.type);
    ui->transmittableBox->setCurrentIndex(p.transmittable ? 0 : 1);
    ui->descriptionEdit->document()->setHtml(p.description);
    ui->cDefineEdit->setText(p.cDefine);

    // Double

    ui->doubleEditorScaleBox->setValue(p.editorScale);
    ui->doubleEditPercentageBox->setChecked(p.editAsPercentage);
    ui->doubleDecimalsBox->setValue(p.editorDecimalsDouble);
    ui->doubleMaxBox->setValue(p.maxDouble);
    ui->doubleMinBox->setValue(p.minDouble);
    ui->doubleShowDisplayBox->setChecked(p.showDisplay);
    ui->doubleStep->setValue(p.stepDouble);
    ui->doubleSuffixEdit->setText(p.suffix);

    double val = p.vTxDoubleScale;
    double power = floor(log10(val));
    val /= pow(10, power);

    ui->doubleTxScaleBaseBox->setValue(val);
    ui->doubleTxScaleExpBox->setValue(power);

    if (p.vTx == VESC_TX_DOUBLE16) {
        ui->doubleTxTypeBox->setCurrentIndex(0);
    } else if (p.vTx == VESC_TX_DOUBLE32) {
        ui->doubleTxTypeBox->setCurrentIndex(1);
    } else {
        ui->doubleTxTypeBox->setCurrentIndex(2);
    }

    ui->doubleValBox->setValue(p.valDouble);

    // Int
    ui->intEditorScaleBox->setValue(p.editorScale);
    ui->intEditPercentageBox->setChecked(p.editAsPercentage);
    ui->intMaxBox->setValue(p.maxInt);
    ui->intMinBox->setValue(p.minInt);
    ui->intShowDisplayBox->setChecked(p.showDisplay);
    ui->intStepBox->setValue(p.stepInt);
    ui->intSuffixEdit->setText(p.suffix);
    if (p.vTx <= 6 && p.vTx > 0) {
        ui->intTxTypeBox->setCurrentIndex(p.vTx - 1);
    }
    ui->intValBox->setValue(p.valInt);

    // String
    ui->stringValEdit->setText(p.valString);

    // Enum
    ui->enumList->clear();
    for (int i = 0;i < p.enumNames.size();i++) {
        addEnum(p.enumNames.at(i));
    }

    if (p.valInt >= 0 && p.valInt < p.enumNames.size()) {
        ui->enumList->setCurrentRow(p.valInt);
    }

    // Bool
    ui->boolBox->setCurrentIndex(p.valInt > 0 ? 1 : 0);
}

QString ParameterEditor::getEditorValues(ConfigParam *p)
{
    if (p) {
        p->longName = ui->longNameEdit->text();
        p->type = CFG_T(ui->typeBox->currentIndex());
        p->transmittable = ui->transmittableBox->currentIndex() == 0;
        p->description = ui->descriptionEdit->toHtmlNoFontSize();
        p->cDefine = ui->cDefineEdit->text();

        switch (p->type) {
        case CFG_T_DOUBLE:
            p->editorScale = ui->doubleEditorScaleBox->value();
            p->editAsPercentage = ui->doubleEditPercentageBox->isChecked();
            p->editorDecimalsDouble = ui->doubleDecimalsBox->value();
            p->maxDouble = ui->doubleMaxBox->value();
            p->minDouble = ui->doubleMinBox->value();
            p->showDisplay = ui->doubleShowDisplayBox->isChecked();
            p->stepDouble = ui->doubleStep->value();
            p->suffix = ui->doubleSuffixEdit->text();
            p->vTxDoubleScale = ui->doubleTxScaleBaseBox->value() *
                    pow(10, ui->doubleTxScaleExpBox->value());
            p->vTx = VESC_TX_T(ui->doubleTxTypeBox->currentIndex() + 7);
            p->valDouble = ui->doubleValBox->value();
            break;

        case CFG_T_INT:
            p->editorScale = ui->intEditorScaleBox->value();
            p->editAsPercentage = ui->intEditPercentageBox->isChecked();
            p->maxInt = ui->intMaxBox->value();
            p->minInt = ui->intMinBox->value();
            p->showDisplay = ui->intShowDisplayBox->isChecked();
            p->stepInt = ui->intStepBox->value();
            p->suffix = ui->intSuffixEdit->text();
            p->vTx = VESC_TX_T(ui->intTxTypeBox->currentIndex() + 1);
            p->valInt = ui->intValBox->value();
            break;

        case CFG_T_QSTRING:
            p->valString = ui->stringValEdit->text();
            break;

        case CFG_T_ENUM:
            p->enumNames.clear();
            for (int i = 0;i < ui->enumList->count();i++) {
                p->enumNames.append(ui->enumList->item(i)->text());
            }
            p->valInt = ui->enumList->currentRow();
            break;

        case CFG_T_BOOL:
            p->valInt = ui->boolBox->currentIndex();
            break;

        default:
            break;
        }
    }

    return ui->nameEdit->text();
}

void ParameterEditor::addEnum(QString name)
{
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setFlags (item->flags() | Qt :: ItemIsEditable);
    ui->enumList->addItem(item);
}

void ParameterEditor::showStatusInfo(QString info, bool isGood)
{
    if (isGood) {
        mStatusLabel->setStyleSheet("QLabel { background-color : lightgreen; color : black; }");
    } else {
        mStatusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
    }

    mStatusInfoTime = 80;
    mStatusLabel->setText(info);
}

void ParameterEditor::updateGroupList()
{
    int index = -1;
    if (ui->groupList->selectedItems().size() > 0) {
        index = ui->groupList->currentRow();
    }

    ui->groupList->clearSelection();
    ui->groupList->clear();
    ui->groupList->addItems(mParams.getParamGroups());
    if (index >= 0 && index < ui->groupList->count()) {
        ui->groupList->setCurrentRow(index);
    }
}

void ParameterEditor::updateSubgroupList()
{
    int index = -1;
    if (ui->subgroupList->selectedItems().size() > 0) {
        index = ui->subgroupList->currentRow();
    }

    ui->subgroupList->clearSelection();
    ui->subgroupList->clear();

    QListWidgetItem *item = ui->groupList->currentItem();
    if (item) {
        ui->subgroupList->addItems(mParams.getParamSubgroups(item->text()));
        if (index >= 0 && index < ui->subgroupList->count()) {
            ui->subgroupList->setCurrentRow(index);
        }
    }
}

void ParameterEditor::updateGroupParamList()
{
    int index = -1;
    if (ui->groupParamList->selectedItems().size() > 0) {
        index = ui->groupParamList->currentRow();
    }

    ui->groupParamList->clearSelection();
    ui->groupParamList->clear();

    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();

    if (itemGroup && itemSubgroup) {
        ui->groupParamList->addItems(mParams.getParamsFromSubgroup(itemGroup->text(), itemSubgroup->text()));
        if (index >= 0 && index < ui->groupParamList->count()) {
            ui->groupParamList->setCurrentRow(index);
        }
    }
}

void ParameterEditor::on_doubleTxTypeBox_currentIndexChanged(int index)
{
    if (index == 2) {
        ui->doubleTxScaleBaseBox->setEnabled(false);
        ui->doubleTxScaleExpBox->setEnabled(false);
        ui->doubleTxScaleExpLabel->setEnabled(false);
        ui->doubleTxScaleLabel->setEnabled(false);
    } else {
        ui->doubleTxScaleBaseBox->setEnabled(true);
        ui->doubleTxScaleExpBox->setEnabled(true);
        ui->doubleTxScaleExpLabel->setEnabled(true);
        ui->doubleTxScaleLabel->setEnabled(true);
    }
}

void ParameterEditor::on_actionCalculatePacketSize_triggered()
{
    VByteArray bytes;
    mParams.serialize(bytes);
    QMessageBox::information(this,
                             tr("Packet Size"),
                             tr("%1 Bytes").arg(bytes.size()));
}

void ParameterEditor::on_groupRemoveButton_clicked()
{
    QListWidgetItem *item = ui->groupList->currentItem();

    if (item) {
        mParams.removeParamGroup(item->text());
        ui->groupList->clearSelection();
        updateGroupList();
    }
}

void ParameterEditor::on_groupDownButton_clicked()
{
    QListWidgetItem *item = ui->groupList->currentItem();

    if (item) {
        if (ui->groupList->currentRow() < (ui->groupList->count() - 1)) {
            mParams.moveGroupDown(item->text());
            ui->groupList->setCurrentRow(ui->groupList->currentRow() + 1);
            updateGroupList();
        }
    }
}

void ParameterEditor::on_groupUpButton_clicked()
{
    QListWidgetItem *item = ui->groupList->currentItem();

    if (item) {
        if (ui->groupList->currentRow() > 0) {
            mParams.moveGroupUp(item->text());
            ui->groupList->setCurrentRow(ui->groupList->currentRow() - 1);
            updateGroupList();
        }
    }
}

void ParameterEditor::on_groupAddButton_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Add Group",
                                         "Name:", QLineEdit::Normal,
                                         "", &ok);
    if (ok && text.size() > 0) {
        mParams.addParamGroup(text);
        updateGroupList();
        ui->groupList->setCurrentRow(ui->groupList->count() - 1);
    }
}

void ParameterEditor::on_groupEditButton_clicked()
{
    QListWidgetItem *item = ui->groupList->currentItem();

    if (item) {
        bool ok;
        QString text = QInputDialog::getText(this, "Rename Group",
                                             "New name:", QLineEdit::Normal,
                                             item->text(), &ok);
        if (ok && text.size() > 0) {
            mParams.renameGroup(item->text(), text);
            updateGroupList();
        }
    }
}

void ParameterEditor::on_subgroupRemoveButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();

    if (itemGroup && itemSubgroup) {
        mParams.removeParamSubgroup(itemGroup->text(), itemSubgroup->text());
        ui->subgroupList->clearSelection();
        updateSubgroupList();
    }
}

void ParameterEditor::on_subgroupDownButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();

    if (itemGroup && itemSubgroup) {
        if (ui->subgroupList->currentRow() < (ui->subgroupList->count() - 1)) {
            mParams.moveSubgroupDown(itemGroup->text(), itemSubgroup->text());
            ui->subgroupList->setCurrentRow(ui->subgroupList->currentRow() + 1);
            updateSubgroupList();
        }
    }
}

void ParameterEditor::on_subgroupUpButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();

    if (itemGroup && itemSubgroup) {
        if (ui->subgroupList->currentRow() > 0) {
            mParams.moveSubgroupUp(itemGroup->text(), itemSubgroup->text());
            ui->subgroupList->setCurrentRow(ui->subgroupList->currentRow() - 1);
            updateSubgroupList();
        }
    }
}

void ParameterEditor::on_subgroupAddButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();

    if (itemGroup) {
        bool ok;
        QString text = QInputDialog::getText(this, "Add Subgroup",
                                             "Name:", QLineEdit::Normal,
                                             "", &ok);
        if (ok && text.size() > 0) {
            mParams.addParamSubgroup(itemGroup->text(), text);
            updateSubgroupList();
            ui->subgroupList->setCurrentRow(ui->subgroupList->count() - 1);
        }
    }
}

void ParameterEditor::on_subgroupEditButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();

    if (itemGroup && itemSubgroup) {
        bool ok;
        QString text = QInputDialog::getText(this, "Rename Subgroup",
                                             "Name:", QLineEdit::Normal,
                                             itemSubgroup->text(), &ok);
        if (ok && text.size() > 0) {
            mParams.renameSubgroup(itemGroup->text(), itemSubgroup->text(), text);
            updateSubgroupList();
        }
    }
}

void ParameterEditor::on_groupParamRemoveButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();
    QListWidgetItem *itemGroupParam = ui->groupParamList->currentItem();

    if (itemGroup && itemSubgroup && itemGroupParam) {
        mParams.removeParamFromSubgroup(itemGroup->text(), itemSubgroup->text(), itemGroupParam->text());
        ui->groupParamList->clearSelection();
        updateGroupParamList();
    }
}

void ParameterEditor::on_groupParamDownButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();
    QListWidgetItem *itemGroupParam = ui->groupParamList->currentItem();

    if (itemGroup && itemSubgroup && itemGroupParam) {
        if (ui->groupParamList->currentRow() < (ui->groupParamList->count() - 1)) {
            mParams.moveSubgroupParamDown(itemGroup->text(), itemSubgroup->text(), itemGroupParam->text());
            ui->groupParamList->setCurrentRow(ui->groupParamList->currentRow() + 1);
            updateGroupParamList();
        }
    }
}

void ParameterEditor::on_groupParamUpButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();
    QListWidgetItem *itemGroupParam = ui->groupParamList->currentItem();

    if (itemGroup && itemSubgroup && itemGroupParam) {
        if (ui->groupParamList->currentRow() > 0) {
            mParams.moveSubgroupParamUp(itemGroup->text(), itemSubgroup->text(), itemGroupParam->text());
            ui->groupParamList->setCurrentRow(ui->groupParamList->currentRow() - 1);
            updateGroupParamList();
        }
    }
}

void ParameterEditor::on_groupParamAddButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();

    if (itemGroup && itemSubgroup) {
        QListWidgetItem *item = ui->paramList->currentItem();
        QString selected = "";

        if (item) {
            selected = item->text();
        }

        bool ok;
        QString text = QInputDialog::getText(this, "Add Parameter",
                                             "Name:", QLineEdit::Normal,
                                             selected, &ok);
        if (ok && text.size() > 0) {
            mParams.addParamToSubgroup(itemGroup->text(), itemSubgroup->text(), text);
            updateGroupParamList();
            ui->groupParamList->setCurrentRow(ui->groupParamList->count() - 1);
        }
    }
}

void ParameterEditor::on_groupParamEditButton_clicked()
{
    QListWidgetItem *itemGroup = ui->groupList->currentItem();
    QListWidgetItem *itemSubgroup = ui->subgroupList->currentItem();
    QListWidgetItem *itemGroupParam = ui->groupParamList->currentItem();

    if (itemGroup && itemSubgroup && itemGroupParam) {
        bool ok;
        QString text = QInputDialog::getText(this, "Change Parameter",
                                             "Name:", QLineEdit::Normal,
                                             itemGroupParam->text(), &ok);
        if (ok && text.size() > 0) {
            mParams.renameSubgroupParam(itemGroup->text(), itemSubgroup->text(), itemGroupParam->text(), text);
            updateGroupParamList();
        }
    }
}
