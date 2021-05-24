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

#ifndef STARTUPWIZARD_H
#define STARTUPWIZARD_H

#include <QObject>
#include <QWizard>
#include <QSettings>
#include <QLabel>
#include <QCheckBox>

#include "vescinterface.h"
#include "widgets/vtextbrowser.h"
#include "widgets/aspectimglabel.h"

class StartupWizard : public QWizard
{
    Q_OBJECT
public:
    enum {
        Page_Intro = 0,
        Page_Usage,
        Page_Warranty,
        Page_Conclusion
    };

    explicit StartupWizard(VescInterface *vesc, QWidget *parent = 0);

private slots:
    void idChanged(int id);

private:
    AspectImgLabel *mSideLabel;
};

class StartupIntroPage : public QWizardPage
{
    Q_OBJECT

public:
    StartupIntroPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    VTextBrowser *mBrowser;
};

class StartupUsagePage : public QWizardPage
{
    Q_OBJECT

public:
    StartupUsagePage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private slots:
    void scrollValueChanged(int value);
    void scrollRangeChanged();

private:
    VTextBrowser *mBrowser;
    QCheckBox *mAcceptBox;

};

class StartupWarrantyPage : public QWizardPage
{
    Q_OBJECT

public:
    StartupWarrantyPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private slots:
    void scrollValueChanged(int value);
    void scrollRangeChanged();

private:
    VTextBrowser *mBrowser;
    QCheckBox *mAcceptBox;

};

class StartupConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    StartupConclusionPage(VescInterface *vesc, QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;
    bool validatePage() Q_DECL_OVERRIDE;

private:
    VTextBrowser *mBrowser;

};

#endif // STARTUPWIZARD_H
