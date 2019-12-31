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

#include "vesc3dview.h"

#include <QTimer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <cmath>
#include <QPainter>

Vesc3DView::Vesc3DView(QWidget *parent) : QOpenGLWidget(parent)
{
    mXRot = 0;
    mYRot = 0;
    mZRot = 0;

    mQ0 = 1.0;
    mQ1 = 0.0;
    mQ2 = 0.0;
    mQ3 = 0.0;
    mUseQuaternions = false;

    mBgColor = palette().color(QPalette::Window);

    memset(mTextures, 0, sizeof(mTextures));
    mProgram = nullptr;

    // Multisampling
    QSurfaceFormat format;
    format.setSamples(4);
    setFormat(format);
}

Vesc3DView::~Vesc3DView()
{
    makeCurrent();
    mVbo.destroy();
    for (int i = 0; i < 6; ++i) {
        delete mTextures[i];
    }
    delete mProgram;
    doneCurrent();
}

QSize Vesc3DView::minimumSizeHint() const
{
    return QSize(0, 0);
}

QSize Vesc3DView::sizeHint() const
{
    return QSize(0, 0);
}

void Vesc3DView::setRollPitchYaw(double roll, double pitch, double yaw)
{
    mXRot = float(roll);
    mYRot = float(pitch);
    mZRot = float(yaw);
    mUseQuaternions = false;
    updateUsingTimer();
}

void Vesc3DView::setQuanternions(float q0, float q1, float q2, float q3)
{
    mQ0 = q0;
    mQ1 = q1;
    mQ2 = q2;
    mQ3 = q3;
    mUseQuaternions = true;
    updateUsingTimer();
}

void Vesc3DView::initializeGL()
{
    initializeOpenGLFunctions();

    makeObject();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
            "attribute highp vec4 vertex;\n"
            "attribute mediump vec4 texCoord;\n"
            "varying mediump vec4 texc;\n"
            "uniform mediump mat4 matrix;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = matrix * vertex;\n"
            "    texc = texCoord;\n"
            "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
            "uniform sampler2D texture;\n"
            "varying mediump vec4 texc;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = texture2D(texture, texc.st);\n"
            "}\n";
    fshader->compileSourceCode(fsrc);

    mProgram = new QOpenGLShaderProgram;
    mProgram->addShader(vshader);
    mProgram->addShader(fshader);
    mProgram->bindAttributeLocation("vertex", 0);
    mProgram->bindAttributeLocation("texCoord", 1);
    mProgram->link();

    mProgram->bind();
    mProgram->setUniformValue("texture", 0);
}

void Vesc3DView::paintGL()
{
    glClearColor(mBgColor.redF(), mBgColor.greenF(), mBgColor.blueF(), mBgColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    double aspect = double(width()) / double(height() ? height() : 1);
    m.perspective(30.0 / qMin(aspect, 1.0), aspect, 0.5, 5.0);
    m.translate(0.0f, 0.0f, -1.2f);
    m.rotate(180, 0.0f, 1.0f, 0.0f);
    m.rotate(180, 0.0f, 0.0f, 1.0f);

    if (mUseQuaternions) {
        // TODO: Test if this works...
        m.rotate(QQuaternion(mQ0, mQ1, mQ2, mQ3));
    } else {
        m.rotate(mZRot, 0.0f, 1.0f, 0.0f);
        m.rotate(mYRot, 1.0f, 0.0f, 0.0f);
        m.rotate(mXRot, 0.0f, 0.0f, 1.0f);
    }
    m.scale(1.0, 0.2, 1.0);
    mProgram->setUniformValue("matrix", m);

    mProgram->enableAttributeArray(0);
    mProgram->enableAttributeArray(1);
    mProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    mProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    for (int i = 0; i < 6; ++i) {
        mTextures[i]->bind();
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }
}

void Vesc3DView::makeObject()
{
    static const int coords[6][4][3] = {
        { { +1, -1, -1 }, { -1, -1, -1 }, { -1, +1, -1 }, { +1, +1, -1 } },
        { { +1, +1, -1 }, { -1, +1, -1 }, { -1, +1, +1 }, { +1, +1, +1 } },
        { { +1, -1, +1 }, { +1, -1, -1 }, { +1, +1, -1 }, { +1, +1, +1 } },
        { { -1, -1, -1 }, { -1, -1, +1 }, { -1, +1, +1 }, { -1, +1, -1 } },
        { { +1, -1, +1 }, { -1, -1, +1 }, { -1, -1, -1 }, { +1, -1, -1 } },
        { { -1, -1, +1 }, { +1, -1, +1 }, { +1, +1, +1 }, { -1, +1, +1 } }
    };

    QImage imgSq(512, 512, QImage::Format_RGB32);
    imgSq.fill(Qt::darkGray);
    QPainter *p = new QPainter(&imgSq);
    QPen pen;
    pen.setWidth(6);
    pen.setColor(QColor(1, 1, 1));
    p->setPen(pen);
    p->drawRect(0, 0, 512, 512);
    delete p;

    QImage imgSide(512, 102, QImage::Format_RGB32);
    imgSide.fill(Qt::darkGray);
    p = new QPainter(&imgSide);
    p->setPen(pen);
    p->drawRect(0, 0, 512, 102);
    delete p;

    mTextures[0] = new QOpenGLTexture(imgSide);
    mTextures[1] = new QOpenGLTexture(imgSq);
    mTextures[2] = new QOpenGLTexture(imgSide);
    mTextures[3] = new QOpenGLTexture(imgSide);
    mTextures[4] = new QOpenGLTexture(QImage(QString(":/res/images/v6plus_top.png")).mirrored());
    mTextures[5] = new QOpenGLTexture(imgSide);

    for (int i = 0;i < 6;i++) {
        mTextures[i]->setMinificationFilter(QOpenGLTexture::Linear);
        mTextures[i]->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    QVector<GLfloat> vertData;
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 4; ++j) {
            // vertex position
            vertData.append(0.2 * coords[i][j][0]);
            vertData.append(0.2 * coords[i][j][1]);
            vertData.append(0.2 * coords[i][j][2]);
            // texture coordinate
            vertData.append(j == 0 || j == 3);
            vertData.append(j == 0 || j == 1);
        }
    }

    mVbo.create();
    mVbo.bind();
    mVbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}

void Vesc3DView::updateUsingTimer()
{
    QTimer::singleShot(1, this, [this]() {
        update();
    });
}
