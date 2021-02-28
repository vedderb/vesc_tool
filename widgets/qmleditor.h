#ifndef QMLEDITOR_H
#define QMLEDITOR_H

#include <QWidget>
#include <QCodeEditor>

namespace Ui {
class QmlEditor;
}

class QmlEditor : public QWidget
{
    Q_OBJECT

public:
    explicit QmlEditor(QWidget *parent = nullptr);
    ~QmlEditor();

    QCodeEditor *editor();
    QString fileNow();
    void setFileNow(QString fileName);

protected:
    void keyPressEvent(QKeyEvent *event);

signals:
    void fileOpened(QString fileName);
    void fileSaved(QString fileName);
    void fileNameChanged(QString newName);

private slots:
    void on_openFileButton_clicked();
    void on_saveButton_clicked();
    void on_saveAsButton_clicked();
    void on_searchEdit_textChanged(const QString &arg1);
    void on_searchPrevButton_clicked();
    void on_searchNextButton_clicked();
    void on_replaceThisButton_clicked();
    void on_replaceAllButton_clicked();
    void on_searchHideButton_clicked();
    void on_searchCaseSensitiveBox_toggled(bool checked);

private:
    Ui::QmlEditor *ui;

};

#endif // QMLEDITOR_H
