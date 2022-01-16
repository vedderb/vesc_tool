#include "setIosParameters.h"

#ifdef Q_OS_IOS
#include <CoreLocation/CoreLocation.h>
#include <UIKit/UIKit.h>
SetIosParams::SetIosParams()
{
    //Top
    //UIView *statusBar = [[UIView alloc]initWithFrame:[UIApplication sharedApplication].keyWindow.windowScene.statusBarManager.statusBarFrame] ;
   // statusBar.backgroundColor = [UIColor colorWithRed:80.0/255.0 green:80.0/255.0 blue:80.0/255.0 alpha:0.0];
    //[[UIApplication sharedApplication] statusBarFrame].size.height = 10;
   // [[UIApplication sharedApplication].keyWindow addSubview:statusBar];

    //Bottom
   // UIApplication *app = [UIApplication sharedApplication];
    //app.windows.firstObject.backgroundColor = [UIColor colorWithRed:80.0/255.0 green:80.0/255.0 blue:80.0/255.0 alpha:1.0];
}


void SetIosParams::NoSleep()
{
    // Configure the phone to application to keep the phone screen awake
       [UIApplication sharedApplication].idleTimerDisabled = YES;
}
#endif
