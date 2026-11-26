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

#include "pagewelcome.h"
#include "ui_pagewelcome.h"
#include "setupwizardmotor.h"
#include "setupwizardapp.h"
#include "widgets/detectallfocdialog.h"

#include <QMessageBox>
#include <QQmlEngine>
#include <QQmlContext>
#include <QSettings>
#include <QmlHighlighter>
#include <QQuickItem>
#include <QLibraryInfo>
#include <QVersionNumber>
#include <QQuickWindow>
#include <QNativeGestureEvent>

PageWelcome::PageWelcome(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageWelcome)
{
    ui->setupUi(this);

    // Qt 6.12 maps QQuickWidget mouse positions wrong unless it is a native window, and on Windows the touchpad
    // gestures from wintouchpad.cpp need one. The other attributes keep the widgets around it alien.
    bool nativeWindow = QLibraryInfo::version() >= QVersionNumber(6, 12);
#ifdef Q_OS_WIN
    nativeWindow = true;
#endif
    if (nativeWindow) {
        QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        ui->qmlWidget->setAttribute(Qt::WA_DontCreateNativeAncestors);
        ui->qmlWidget->setAttribute(Qt::WA_NativeWindow);
    }

    // QQuickWidget does not forward touchpad pinches
    ui->qmlWidget->installEventFilter(this);

    mUtil = new Utility(this);

    ui->autoConnectButton->setIcon(Utility::getIcon("icons/Connected-96.png"));
    ui->wizardFocSimpleButton->setIcon(Utility::getIcon("icons/Wizard-96.png"));
    ui->wizardAppButton->setIcon(Utility::getIcon("icons/Wizard-96.png"));
    ui->nrfPairButton->setIcon(Utility::getIcon("icons/icons8-fantasy-96.png"));
    ui->multiSettingButton->setIcon(Utility::getIcon("icons/Settings-96.png"));
    ui->invertDirButton->setIcon(Utility::getIcon("icons/Process-96.png"));
    ui->setupBluetoothButton->setIcon(Utility::getIcon("icons/bluetooth.png"));
    ui->wizardIMUButton->setIcon(Utility::getIcon("icons/imu_off.png"));

    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;
    ui->bgWidget->setPixmap(QPixmap("://res/bg.png"));

    connect(ui->wizardFocSimpleButton, &QPushButton::clicked, [this]() {
        QMetaObject::invokeMethod(ui->qmlWidget->rootObject(), "setupMotors");
    });

    connect(ui->wizardAppButton, SIGNAL(clicked(bool)),
            this, SLOT(startSetupWizardApp()));

    connect(ui->multiSettingButton, &QPushButton::clicked, [this]() {
        QMetaObject::invokeMethod(ui->qmlWidget->rootObject(), "openMultiSettings");
    });

    connect(ui->invertDirButton, &QPushButton::clicked, [this]() {
        QMetaObject::invokeMethod(ui->qmlWidget->rootObject(), "dirSetup");
    });
}

bool PageWelcome::eventFilter(QObject *object, QEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    if (object == ui->qmlWidget && event->type() == QEvent::NativeGesture) {
        auto e = static_cast<QNativeGestureEvent*>(event);
        QNativeGestureEvent mapped(e->gestureType(), e->pointingDevice(), e->fingerCount(), e->position(),
                                   e->position(), e->globalPosition(), e->value(), e->delta());
        QCoreApplication::sendEvent(ui->qmlWidget->quickWindow(), &mapped);
        return true;
    }
#endif
    return QWidget::eventFilter(object, event);
}

PageWelcome::~PageWelcome()
{
    delete ui;
}

void PageWelcome::startSetupWizardFocSimple()
{
    if (mVesc) {
        DetectAllFocDialog::showDialog(mVesc, this);
    }
}

void PageWelcome::startSetupWizardFocQml()
{
    if (mVesc) {
        mQmlUi.startCustomGui(mVesc, "qrc:/res/qml/SetupMotorWindow.qml");
    }
}

void PageWelcome::startSetupWizardMotor()
{
    if (mVesc) {
        SetupWizardMotor w(mVesc, this);
        w.exec();
    }
}

void PageWelcome::startSetupWizardApp()
{
    if (mVesc) {
        SetupWizardApp w(mVesc, this);
        w.exec();
    }
}

VescInterface *PageWelcome::vesc() const
{
    return mVesc;
}

void PageWelcome::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    ui->qmlWidget->engine()->rootContext()->setContextProperty("VescIf", mVesc);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("QmlUi", this);
    ui->qmlWidget->engine()->rootContext()->setContextProperty("Utility", mUtil);

    ui->qmlWidget->setSource(QUrl(QLatin1String("qrc:/res/qml/WelcomeQmlPanel.qml")));
}

void PageWelcome::on_autoConnectButton_clicked()
{
    Utility::autoconnectBlockingWithProgress(mVesc, this);
}

void PageWelcome::on_nrfPairButton_clicked()
{
    QMetaObject::invokeMethod(ui->qmlWidget->rootObject(), "nrfQuickPair");
}

void PageWelcome::on_setupBluetoothButton_clicked()
{
    QMetaObject::invokeMethod(ui->qmlWidget->rootObject(), "openBleSetup");
}

void PageWelcome::on_wizardIMUButton_clicked()
{
    QMetaObject::invokeMethod(ui->qmlWidget->rootObject(), "openWizardIMU");
}

