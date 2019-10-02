/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

#ifndef VESC3DVIEW_H
#define VESC3DVIEW_H

#include <QObject>
#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class Vesc3DView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit Vesc3DView(QWidget *parent = nullptr);
    ~Vesc3DView();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setRollPitchYaw(double roll, double pitch, double yaw);
    void setQuanternions(float q0, float q1, float q2, float q3);

protected:
    void initializeGL();
    void paintGL();

private:
    float mXRot;
    float mYRot;
    float mZRot;
    float mScale;
    float mQ0, mQ1, mQ2, mQ3;
    bool mUseQuaternions;
    QPoint mLastPos;
    QColor mBgColor;

    QOpenGLTexture *mTextures[6];
    QOpenGLShaderProgram *mProgram;
    QOpenGLBuffer mVbo;

    void makeObject();
    void updateUsingTimer();

};

#endif // VESC3DVIEW_H
