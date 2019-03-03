/*
    Copyright 2016 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#ifndef SETUPWIZARDAPP_H
#define SETUPWIZARDAPP_H

#include <QWizard>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <QPushButton>

#include "vescinterface.h"
#include "widgets/paramtable.h"
#include "widgets/nrfpair.h"
#include "widgets/displaypercentage.h"
#include "pages/pageconnection.h"
#include "widgets/ppmmap.h"
#include "widgets/adcmap.h"
#include "widgets/aspectimglabel.h"

class SetupWizardApp : public QWizard
{
    Q_OBJECT

public:
    enum {
        Page_Intro = 0,
        Page_Connection,
        Page_Firmware,
        Page_Multi,
        Page_General,
        Page_Nunchuk,
        Page_Ppm_Map,
        Page_Ppm,
        Page_Adc_Map,
        Page_Adc,
        Page_Conclusion
    };

    enum {
        Input_Ppm = 0,
        Input_Adc,
        Input_Nunchuk,
        Input_NunchukNrf
    };

    SetupWizardApp(VescInterface *vesc, QWidget *parent = 0);

private slots:
    void idChanged(int id);
    void ended();

private:
    AspectImgLabel *mSideLabel;
    VescInterface *mVesc;
    bool mCanLastFwd;
    int mCanLastId;

};

class AppIntroPage : public QWizardPage
{
    Q_OBJECT

public:
    AppIntroPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;

private:
    VescInterface *mVesc;
    QLabel *mLabel;
    bool mResetInputOk;

};

class AppConnectionPage : public QWizardPage
{
    Q_OBJECT

public:
    AppConnectionPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool isComplete() const Q_DECL_OVERRIDE;

private:
    VescInterface *mVesc;
    PageConnection *mPageConnection;

};

class AppFirmwarePage : public QWizardPage
{
    Q_OBJECT

public:
    AppFirmwarePage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    VescInterface *mVesc;
    QLabel *mLabel;

};

class AppMultiPage : public QWizardPage
{
    Q_OBJECT

public:
    AppMultiPage(VescInterface *vesc, QWidget *parent = 0);
    void initializePage() Q_DECL_OVERRIDE;
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void cleanupPage() Q_DECL_OVERRIDE;

    Q_PROPERTY(int canFwd READ getCanFwd NOTIFY canFwdChanged)

    int getCanFwd();

signals:
    int canFwdChanged();

private:
    VescInterface *mVesc;
    QListWidget *mCanFwdList;

};

class AppGeneralPage : public QWizardPage
{
    Q_OBJECT

public:
    AppGeneralPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void cleanupPage() Q_DECL_OVERRIDE;

    Q_PROPERTY(int inputType READ getInputType NOTIFY inputTypeChanged)

    int getInputType();

signals:
    void inputTypeChanged();

private:
    VescInterface *mVesc;
    QListWidget *mInputList;

};

class AppNunchukPage : public QWizardPage
{
    Q_OBJECT

public:
    AppNunchukPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void initializePage() Q_DECL_OVERRIDE;

private slots:
    void decodedChukReceived(double value);
    void timerSlot();

private:
    VescInterface *mVesc;
    ParamTable *mParamTab;
    NrfPair *mNrfPair;
    DisplayPercentage *mDisplay;
    QTimer *mTimer;
    QPushButton *mWriteButton;

};

class AppPpmMapPage : public QWizardPage
{
    Q_OBJECT

public:
    AppPpmMapPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void initializePage() Q_DECL_OVERRIDE;

private slots:
    void paramChangedDouble(QObject *src, QString name, double newParam);
    void timerSlot();

private:
    VescInterface *mVesc;
    ParamTable *mParamTab;
    PpmMap *mPpmMap;
    QTimer *mTimer;

};

class AppPpmPage : public QWizardPage
{
    Q_OBJECT

public:
    AppPpmPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void initializePage() Q_DECL_OVERRIDE;

private:
    VescInterface *mVesc;
    ParamTable *mParamTab;
    QPushButton *mWriteButton;

};

class AppAdcMapPage : public QWizardPage
{
    Q_OBJECT

public:
    AppAdcMapPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void initializePage() Q_DECL_OVERRIDE;

private slots:
    void paramChangedDouble(QObject *src, QString name, double newParam);
    void paramChangedBool(QObject *src, QString name, bool newParam);
    void timerSlot();

private:
    VescInterface *mVesc;
    ParamTable *mParamTab;
    AdcMap *mAdcMap;
    QTimer *mTimer;

};

class AppAdcPage : public QWizardPage
{
    Q_OBJECT

public:
    AppAdcPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;
    void initializePage() Q_DECL_OVERRIDE;

private:
    VescInterface *mVesc;
    ParamTable *mParamTab;
    QPushButton *mWriteButton;

};

class AppConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    AppConclusionPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    void initializePage() Q_DECL_OVERRIDE;

private:
    VescInterface *mVesc;
    QLabel *mLabel;

};

#endif // SETUPWIZARDAPP_H
