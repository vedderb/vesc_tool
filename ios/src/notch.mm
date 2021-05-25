#include <UIKit/UIKit.h>
#include "notch.h"

Notch::Notch()
{
    //Top
    UIView *statusBar = [[UIView alloc]initWithFrame:[UIApplication sharedApplication].keyWindow.windowScene.statusBarManager.statusBarFrame] ;
    statusBar.backgroundColor = [UIColor colorWithRed:80.0/255.0 green:80.0/255.0 blue:80.0/255.0 alpha:1.0];
    [[UIApplication sharedApplication].keyWindow addSubview:statusBar];

    //Bottom
    UIApplication *app = [UIApplication sharedApplication];
    app.windows.firstObject.backgroundColor = [UIColor colorWithRed:80.0/255.0 green:80.0/255.0 blue:80.0/255.0 alpha:1.0];
}
