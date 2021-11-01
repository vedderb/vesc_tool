#ifndef SETIOSPARAMETERS_H
#define SETIOSPARAMETERS_H
#include <QObject>
#include <QApplication>
#ifdef Q_OS_IOS
class SetIosParams : public QObject{
  Q_OBJECT
public:
  SetIosParams();
  void NoSleep();
  void Sleep();
};
#endif
#endif
