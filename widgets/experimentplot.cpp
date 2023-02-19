/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#include <QFileDialog>
#include <QMessageBox>
#include "experimentplot.h"
#include "ui_experimentplot.h"
#include "utility.h"

ExperimentPlot::ExperimentPlot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExperimentPlot)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);

    QIcon mycon = QIcon(Utility::getIcon("icons/expand_on.png"));
    mycon.addPixmap(Utility::getIcon("icons/expand_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_off.png"), QIcon::Normal, QIcon::Off);
    ui->experimentHZoomButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/expand_v_on.png"));
    mycon.addPixmap(Utility::getIcon("icons/expand_v_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/expand_v_off.png"), QIcon::Normal, QIcon::Off);
    ui->experimentVZoomButton->setIcon(mycon);

    mycon = QIcon(Utility::getIcon("icons/size_on.png"));
    mycon.addPixmap(Utility::getIcon("icons/size_on.png"), QIcon::Normal, QIcon::On);
    mycon.addPixmap(Utility::getIcon("icons/size_off.png"), QIcon::Normal, QIcon::Off);
    ui->experimentAutoScaleButton->setIcon(mycon);

    auto genPic = [](QString p1, QString p2, QString text) {
        QPixmap pix(133, 133);
        pix.fill(Qt::transparent);
        QPainter p(&pix);

        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (!p1.isEmpty()) {
            p.drawImage(pix.rect(), Utility::getIcon(p1).toImage());
        }

        if (!p2.isEmpty()) {
            p.drawImage(pix.rect(), Utility::getIcon(p2).toImage());
        }

        if (!text.isEmpty()) {
            QFont font = p.font();
            font.setPixelSize(pix.height());
            font.setBold(true);
            p.setFont(font);
            p.setPen(Utility::getAppQColor("lightText"));
            p.drawText(pix.rect(), Qt::AlignCenter, text);
        }

        return QPixmap(pix);
    };

    auto updateIcon = [genPic](QToolButton *btn, QString pic, QString txt) {
        QIcon mycon = QIcon(genPic(pic, "", txt));
        mycon.addPixmap(genPic("icons/glow.png", pic, txt), QIcon::Normal, QIcon::On);
        mycon.addPixmap(genPic("", pic, txt), QIcon::Normal, QIcon::Off);
        btn->setIcon(mycon);
    };

    updateIcon(ui->experimentGraph1Button, "", "1");
    updateIcon(ui->experimentGraph2Button, "", "2");
    updateIcon(ui->experimentGraph3Button, "", "3");
    updateIcon(ui->experimentGraph4Button, "", "4");
    updateIcon(ui->experimentGraph5Button, "", "5");
    updateIcon(ui->experimentGraph6Button, "", "6");
    updateIcon(ui->experimentShowLineButton, "icons/3ph_sine.png", "");
    updateIcon(ui->experimentScatterButton, "icons/Polyline-96.png", "");

    ui->experimentClearDataButton->setIcon(Utility::getIcon("icons/Delete-96.png"));

    mVesc = 0;
    mExperimentReplot = false;
    mExperimentPlotNow = 0;

    mTimer = new QTimer(this);
    mTimer->start(20);

    Utility::setPlotColors(ui->experimentPlot);
    ui->experimentPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    connect(mTimer, &QTimer::timeout, [=]() {
        if (mExperimentReplot) {
            ui->experimentPlot->clearGraphs();

            for (int i = 0;i < mExperimentPlots.size();i++) {
                switch (i) {
                case 0: if (!ui->experimentGraph1Button->isChecked()) {continue;} break;
                case 1: if (!ui->experimentGraph2Button->isChecked()) {continue;} break;
                case 2: if (!ui->experimentGraph3Button->isChecked()) {continue;} break;
                case 3: if (!ui->experimentGraph4Button->isChecked()) {continue;} break;
                case 4: if (!ui->experimentGraph5Button->isChecked()) {continue;} break;
                case 5: if (!ui->experimentGraph6Button->isChecked()) {continue;} break;
                default: break;
                }

                ui->experimentPlot->addGraph();
                ui->experimentPlot->graph()->setData(mExperimentPlots.at(i).xData, mExperimentPlots.at(i).yData);
                ui->experimentPlot->graph()->setName(mExperimentPlots.at(i).label);

                ui->experimentPlot->graph()->setPen(QPen(mExperimentPlots.at(i).color));
                if (ui->experimentScatterButton->isChecked()) {
                    ui->experimentPlot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
                }
                if (ui->experimentShowLineButton->isChecked()) {
                    ui->experimentPlot->graph()->setLineStyle(QCPGraph::LineStyle::lsLine);
                } else {
                    ui->experimentPlot->graph()->setLineStyle(QCPGraph::LineStyle::lsNone);
                }
            }

            ui->experimentPlot->legend->setVisible(mExperimentPlots.size() > 1);

            if (ui->experimentAutoScaleButton->isChecked()) {
                ui->experimentPlot->rescaleAxes();
            }

            ui->experimentPlot->replotWhenVisible();
            mExperimentReplot = false;
        }
    });

    connect(ui->experimentGraph1Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph2Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph3Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph4Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph5Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentGraph6Button, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentScatterButton, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentShowLineButton, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});
    connect(ui->experimentAutoScaleButton, &QPushButton::toggled,
            [=]() {mExperimentReplot = true;});

    connect(ui->experimentHZoomButton, &QPushButton::toggled, [=]() {
        Qt::Orientations plotOrientations = Qt::Orientations(
                    ((ui->experimentHZoomButton->isChecked() ? Qt::Horizontal : 0) |
                     (ui->experimentVZoomButton->isChecked() ? Qt::Vertical : 0)));
        ui->experimentPlot->axisRect()->setRangeZoom(plotOrientations);
    });

    connect(ui->experimentVZoomButton, &QPushButton::toggled, [=]() {
        Qt::Orientations plotOrientations = Qt::Orientations(
                    ((ui->experimentHZoomButton->isChecked() ? Qt::Horizontal : 0) |
                     (ui->experimentVZoomButton->isChecked() ? Qt::Vertical : 0)));
        ui->experimentPlot->axisRect()->setRangeZoom(plotOrientations);
    });
}

ExperimentPlot::~ExperimentPlot()
{
    delete ui;
}

VescInterface *ExperimentPlot::vesc() const
{
    return mVesc;
}

void ExperimentPlot::setVesc(VescInterface *newVesc)
{
    mVesc = newVesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(plotInitReceived(QString,QString)),
                this, SLOT(plotInitReceived(QString,QString)));
        connect(mVesc->commands(), SIGNAL(plotDataReceived(double,double)),
                this, SLOT(plotDataReceived(double,double)));
        connect(mVesc->commands(), SIGNAL(plotAddGraphReceived(QString)),
                this, SLOT(plotAddGraphReceived(QString)));
        connect(mVesc->commands(), SIGNAL(plotSetGraphReceived(int)),
                this, SLOT(plotSetGraphReceived(int)));
    }
}

void ExperimentPlot::on_experimentLoadXmlButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Load Plot"), "",
                                                    tr("Xml files (*.xml)"));

    if (!filename.isEmpty()) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Load Plot",
                                  "Could not open\n" + filename + "\nfor reading");
            return;
        }

        QXmlStreamReader stream(&file);

        // Look for plot tag
        bool plots_found = false;
        while (stream.readNextStartElement()) {
            if (stream.name() == "plot") {
                plots_found = true;
                break;
            }
        }

        if (plots_found) {
            mExperimentPlots.clear();

            while (stream.readNextStartElement()) {
                QString name = stream.name().toString();

                if (name == "xlabel") {
                    ui->experimentPlot->xAxis->setLabel(stream.readElementText());
                } else if (name == "ylabel") {
                    ui->experimentPlot->yAxis->setLabel(stream.readElementText());
                } else if (name == "graph") {
                    EXPERIMENT_PLOT p;

                    while (stream.readNextStartElement()) {
                        QString name2 = stream.name().toString();

                        if (name2 == "label") {
                            p.label = stream.readElementText();
                        } else if (name2 == "color") {
                            p.color = QColor(stream.readElementText());
                        } else if (name2 == "point") {
                            while (stream.readNextStartElement()) {
                                QString name3 = stream.name().toString();

                                if (name3 == "x") {
                                    p.xData.append(stream.readElementText().toDouble());
                                } else if (name3 == "y") {
                                    p.yData.append(stream.readElementText().toDouble());
                                } else {
                                    qWarning() << ": Unknown XML element :" << name2;
                                    stream.skipCurrentElement();
                                }
                            }
                        } else {
                            qWarning() << ": Unknown XML element :" << name2;
                            stream.skipCurrentElement();
                        }

                        if (stream.hasError()) {
                            qWarning() << " : XML ERROR :" << stream.errorString();
                        }
                    }

                    mExperimentPlots.append(p);
                }

                if (stream.hasError()) {
                    qWarning() << "XML ERROR :" << stream.errorString();
                    qWarning() << stream.lineNumber() << stream.columnNumber();
                }
            }

            mExperimentReplot = true;

            file.close();
            if (mVesc) {
                mVesc->emitStatusMessage("Loaded plot", true);
            }
        } else {
            QMessageBox::critical(this, "Load Plot",
                                  "plot tag not found in " + filename);
        }
    }
}

void ExperimentPlot::on_experimentSaveXmlButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Plot"), "",
                                                    tr("Xml files (*.xml)"));

    if (filename.isEmpty()) {
        return;
    }

    if (!filename.endsWith(".xml", Qt::CaseInsensitive)) {
        filename.append(".xml");
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Save Plot",
                              "Could not open\n" + filename + "\nfor writing");
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setCodec("UTF-8");
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("plot");
    stream.writeTextElement("xlabel", ui->experimentPlot->xAxis->label());
    stream.writeTextElement("ylabel", ui->experimentPlot->yAxis->label());

    foreach (auto p, mExperimentPlots) {
        stream.writeStartElement("graph");
        stream.writeTextElement("label", p.label);
        stream.writeTextElement("color", p.color.name());
        for (int i = 0;i < p.xData.size();i++) {
            stream.writeStartElement("point");
            stream.writeTextElement("x", QString::number(p.xData.at(i)));
            stream.writeTextElement("y", QString::number(p.yData.at(i)));
            stream.writeEndElement();
        }
        stream.writeEndElement();
    }

    stream.writeEndDocument();
    file.close();
}

void ExperimentPlot::on_experimentSavePngButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"), "",
                                                    tr("PNG Files (*.png)"));

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".png", Qt::CaseInsensitive)) {
            fileName.append(".png");
        }

        ui->experimentPlot->savePng(fileName,
                                    ui->experimentWBox->value(),
                                    ui->experimentHBox->value(),
                                    ui->experimentScaleBox->value());
    }
}

void ExperimentPlot::on_experimentSavePdfButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save PDF"), "",
                                                    tr("PDF Files (*.pdf)"));

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
            fileName.append(".pdf");
        }

        ui->experimentPlot->savePdf(fileName,
                                    ui->experimentWBox->value(),
                                    ui->experimentHBox->value());
    }
}

void ExperimentPlot::on_experimentClearDataButton_clicked()
{
    for (auto &d: mExperimentPlots) {
        d.xData.clear();
        d.yData.clear();
    }
    mExperimentReplot = true;
}

void ExperimentPlot::on_experimentSaveCsvButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Plot"), "",
                                                    tr("Csv files (*.csv)"));

    if (filename.isEmpty()) {
        return;
    }

    if (!filename.endsWith(".csv", Qt::CaseInsensitive)) {
        filename.append(".csv");
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Save Plot",
                              "Could not open\n" + filename + "\nfor writing");
        return;
    }

    QTextStream os(&file);

    int maxLen = 0;
    foreach (auto p, mExperimentPlots) {
        os << p.label << " x;" << p.label << " y;";
        if (p.xData.length() > maxLen) {
            maxLen = p.xData.length();
        }
    }
    os << "\n";

    for (int i = 0;i < maxLen;i++) {
        foreach (auto p, mExperimentPlots) {
            if (p.xData.length() > i) {
                os << p.xData.at(i) << ";" << p.yData.at(i) << ";";
            } else {
                os << ";;";
            }
        }
        os << "\n";
    }

    file.close();
}

void ExperimentPlot::plotInitReceived(QString xLabel, QString yLabel)
{
    mExperimentPlots.clear();

    ui->experimentPlot->clearGraphs();
    ui->experimentPlot->xAxis->setLabel(xLabel);
    ui->experimentPlot->yAxis->setLabel(yLabel);

    mExperimentReplot = true;
}

void ExperimentPlot::plotDataReceived(double x, double y)
{
    if (mExperimentPlots.size() <= mExperimentPlotNow) {
        mExperimentPlots.resize(mExperimentPlotNow + 1);
    }

    mExperimentPlots[mExperimentPlotNow].xData.append(x);
    mExperimentPlots[mExperimentPlotNow].yData.append(y);

    int samples = mExperimentPlots[mExperimentPlotNow].xData.size();
    int historyMax = ui->experimentHistoryBox->value();
    if (samples > historyMax) {
        mExperimentPlots[mExperimentPlotNow].xData.remove(0, samples - historyMax);
        mExperimentPlots[mExperimentPlotNow].yData.remove(0, samples - historyMax);
    }

    mExperimentReplot = true;
}

void ExperimentPlot::plotAddGraphReceived(QString name)
{
    mExperimentPlots.resize(mExperimentPlots.size() + 1);
    mExperimentPlots.last().label = name;

    if (mExperimentPlots.size() == 1) {
        mExperimentPlots.last().color = Utility::getAppQColor("plot_graph1");
    } else if (mExperimentPlots.size() == 2) {
        mExperimentPlots.last().color = Utility::getAppQColor("plot_graph2");
    } else if (mExperimentPlots.size() == 3) {
        mExperimentPlots.last().color = Utility::getAppQColor("plot_graph3");
    } else if (mExperimentPlots.size() == 4) {
        mExperimentPlots.last().color = Utility::getAppQColor("plot_graph4");
    } else if (mExperimentPlots.size() == 5) {
        mExperimentPlots.last().color = Utility::getAppQColor("plot_graph5");
    } else {
        mExperimentPlots.last().color = Utility::getAppQColor("plot_graph6");
    }

    mExperimentReplot = true;
}

void ExperimentPlot::plotSetGraphReceived(int graph)
{
    mExperimentPlotNow = graph;
}
