#ifndef BOARDSETUPWINDOW_H
#define BOARDSETUPWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QProcess>
#include <QSettings>
#include <QMap>
#include "vescinterface.h"
#include "widgets/pagelistitem.h"

namespace Ui {
class BoardSetupWindow;
}

class BoardSetupWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BoardSetupWindow(QWidget *parent = nullptr);
    ~BoardSetupWindow();

private slots:
     void timerSlot();
     void showStatusInfo(QString info, bool isGood);
     void showMessageDialog(const QString &title, const QString &msg, bool isGood, bool richText);
     void mcConfigCheckResult(QStringList paramsNotSet);
     void pingCanRx(QVector<int> devs, bool isTimeout);
     void fwUploadStatus(const QString &status, double progress, bool isOngoing);
    // void loadMotorConfig(QString &path);
    // void loadAppConfig(QString &path);

     void on_motorConfigButton_clicked();
     void on_appConfigButton_clicked();
     void on_bleFirmwareButton_clicked();
     void on_serialRefreshButton_clicked();
     void on_startButton_clicked();
     void on_bootloaderCheckBox_stateChanged();
     void on_motorDetectionCheckBox_stateChanged();
     void on_motorTolSlider_valueChanged(int value);
     void on_appCheckBox_stateChanged();
     void on_bleCheckBox_stateChanged();
     void valuesReceived(MC_VALUES values, unsigned int mask);

private:
    Ui::BoardSetupWindow *ui;

    QSettings mSettings;
    QString mVersion;
    QString mcXmlPath;
    QString appXmlPath;
    QString testResultMsg;
    bool testResult;
    VescInterface *mVesc;
    QTimer *mTimer;
    QLabel *mStatusLabel;
    QVector<int> CAN_IDs;
    bool CAN_Timeout;
    bool is_Bootloader;
    bool is_Dual;
    int num_VESCs;
    QString HW_Name;
    MC_VALUES values_now;
    int app_enum_old = 0;

    ConfigParams *mMcConfig_Target;
    ConfigParams *mAppConfig_Target;

    int mStatusInfoTime;
    bool mKeyLeft;
    bool mKeyRight;
    bool mMcConfRead;
    bool mAppConfRead;
    ConfigParams mcConfig_Target;
    QMap<QString, int> mPageNameIdList;

    void uploadFw(bool allOverCan);
    void loadAppConfXML(QString path);
    void loadMotorConfXML(QString path);
    void resetRoutine();

    bool trySerialConnect();
    bool tryBootloaderUpload();
    bool tryFirmwareUpload();
    bool tryBleFirmwareUpload();
    bool tryCANScan();
    bool tryFOCCalibration();
    bool tryTestMotorParameters();
    bool tryMotorDirection();
    bool tryApplySlaveAppSettings();
    bool tryApplyMasterAppSettings();
    bool tryRemoteTest();
    bool tryFinalDiagnostics();
    bool mcConfigOutsideParamBounds(QString paramName, double tolerance);


};
#endif // BOARDSETUPWINDOW_H
