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
#include "qquaternion.h"

#include <QTimer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <cmath>
#include <QFile>
#include <QtEndian>

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

    mTexture = nullptr;
    mProgram = nullptr;
    mVertexCount = 0;

    // Multisampling
    QSurfaceFormat format;
    format.setSamples(4);
    format.setAlphaBufferSize(8);
    setFormat(format);
}

Vesc3DView::~Vesc3DView()
{
    makeCurrent();
    mVbo.destroy();
    delete mTexture;
    delete mProgram;
    doneCurrent();
}

void Vesc3DView::setBgColor(double r, double g, double b, double a)
{
    mBgColor.setRedF(r);
    mBgColor.setGreenF(g);
    mBgColor.setBlueF(b);
    mBgColor.setAlphaF(a);
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
            "attribute mediump vec3 normal;\n"
            "attribute mediump vec3 smoothNormal;\n"
            "attribute mediump vec4 color;\n"
            "uniform highp mat4 matrix;\n"
            "uniform mediump mat3 normalMatrix;\n"
            "uniform mediump vec2 outline;\n"
            "varying mediump vec3 n;\n"
            "varying mediump vec4 c;\n"
            "varying mediump vec2 texc;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = matrix * vertex;\n"
            "    gl_Position.xy += normalize(normalMatrix * smoothNormal).xy * outline * gl_Position.w;\n"
            "    gl_Position.z += (outline.x > 0.0 ? 0.002 : 0.0) * gl_Position.w;\n"
            "    n = normalMatrix * normal;\n"
            "    c = color;\n"
            "    texc = vec2((vertex.x + 54.5) / 109.0, (58.75 - vertex.z) / 117.5);\n"
            "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
            "uniform sampler2D texture;\n"
            "uniform mediump vec2 outline;\n"
            "varying mediump vec3 n;\n"
            "varying mediump vec4 c;\n"
            "varying mediump vec2 texc;\n"
            "void main(void)\n"
            "{\n"
            "    mediump vec3 norm = normalize(n);\n"
            "    mediump float d = dot(norm, normalize(vec3(-0.35, 0.75, 0.55)));\n"
            "    mediump float k = d > 0.55 ? 1.0 : (d > 0.1 ? 0.8 : 0.62);\n"
            "    mediump float rim = 1.0 - abs(norm.z);\n"
            "    mediump vec3 col = mix(c.rgb, texture2D(texture, texc).rgb, c.a) * k + 0.05 * k * k + 0.09 * rim * rim * rim;\n"
            "    gl_FragColor = outline.x > 0.0 ? vec4(0.02, 0.02, 0.025, 1.0) : vec4(col, 1.0);\n"
            "}\n";
    fshader->compileSourceCode(fsrc);

    mProgram = new QOpenGLShaderProgram;
    mProgram->addShader(vshader);
    mProgram->addShader(fshader);
    mProgram->bindAttributeLocation("vertex", 0);
    mProgram->bindAttributeLocation("normal", 1);
    mProgram->bindAttributeLocation("smoothNormal", 2);
    mProgram->bindAttributeLocation("color", 3);
    mProgram->link();

    mProgram->bind();
    mProgram->setUniformValue("texture", 0);
}

void Vesc3DView::paintGL()
{
    glClearColor(mBgColor.redF(), mBgColor.greenF(), mBgColor.blueF(), mBgColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!mProgram || !mProgram->bind()) {
        return;
    }

    if (!mVbo.bind()) {
        mProgram->release();
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    mTexture->bind();

    QMatrix4x4 p;
    double aspect = double(width()) / double(height() ? height() : 1);
    p.perspective(30.0 / qMin(aspect, 1.0), aspect, 0.5, 5.0);

    QMatrix4x4 m;
    m.translate(0.0f, 0.0f, -1.2f);
    m.rotate(30, 1.0f, 0.0f, 0.0f);
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
    mProgram->setUniformValue("normalMatrix", m.normalMatrix());
    m.scale(0.2f / 62.8f);
    mProgram->setUniformValue("matrix", p * m);

    for (int i = 0; i < 4; ++i) {
        mProgram->enableAttributeArray(i);
    }
    mProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 13 * sizeof(GLfloat));
    mProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(GLfloat), 3, 13 * sizeof(GLfloat));
    mProgram->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(GLfloat), 3, 13 * sizeof(GLfloat));
    mProgram->setAttributeBuffer(3, GL_FLOAT, 9 * sizeof(GLfloat), 4, 13 * sizeof(GLfloat));

    // Outline: back faces pushed out along the smoothed normals and slightly back
    float outline = qMin(width(), height()) / 100.0f;
    glCullFace(GL_FRONT);
    mProgram->setUniformValue("outline", QVector2D(outline / width(), outline / height()));
    glDrawArrays(GL_TRIANGLES, 0, mVertexCount);

    glCullFace(GL_BACK);
    mProgram->setUniformValue("outline", QVector2D());
    glDrawArrays(GL_TRIANGLES, 0, mVertexCount);

    for (int i = 0; i < 4; ++i) {
        mProgram->disableAttributeArray(i);
    }
    mVbo.release();
    mProgram->release();
}

void Vesc3DView::makeObject()
{
    // VESC Maxim, baked from the case CAD. res/3d/maxim.bin is qCompress'd:
    // "VMX1", vertex count, triangle count, vertices (int16 xyz in 0.0025 mm,
    // int8 smoothed normal, pad), uint32 indices, uint8 material per triangle.
    static const float colors[][4] = { // rgb, a = top texture
        {0.17f, 0.17f, 0.18f, 0.0f}, // Case
        {0.13f, 0.13f, 0.14f, 0.0f}, // Heatsink
        {0.08f, 0.08f, 0.09f, 1.0f}, // Lid top
        {0.10f, 0.10f, 0.11f, 0.0f}, // Lid edge
        {0.11f, 0.11f, 0.12f, 0.0f}, // Capacitor
        {0.70f, 0.71f, 0.73f, 0.0f}, // Capacitor top
        {0.12f, 0.12f, 0.13f, 0.0f}, // Connector
        {0.68f, 0.70f, 0.72f, 0.0f}, // Screw
        {0.78f, 0.66f, 0.42f, 0.0f}, // Terminal
    };

    QFile file(":/res/3d/maxim.bin");
    file.open(QIODevice::ReadOnly);
    QByteArray data = qUncompress(file.readAll());
    quint32 vertNum = qFromLittleEndian<quint32>(data.constData() + 4);
    quint32 triNum = qFromLittleEndian<quint32>(data.constData() + 8);
    const char *verts = data.constData() + 12;
    const char *inds = verts + vertNum * 10;
    const char *mats = inds + triNum * 12;

    QVector<GLfloat> vertData;
    vertData.reserve(triNum * 3 * 13);
    for (quint32 i = 0; i < triNum; i++) {
        QVector3D pos[3], norm[3];
        for (int j = 0; j < 3; j++) {
            const char *v = verts + 10 * qFromLittleEndian<quint32>(inds + 12 * i + 4 * j);
            pos[j] = QVector3D(qFromLittleEndian<qint16>(v), qFromLittleEndian<qint16>(v + 2),
                               qFromLittleEndian<qint16>(v + 4)) * 0.0025f;
            norm[j] = QVector3D(qint8(v[6]), qint8(v[7]), qint8(v[8])) / 127.0f;
        }

        QVector3D faceNorm = QVector3D::normal(pos[0], pos[1], pos[2]);
        const float *c = colors[quint8(mats[i])];
        for (int j = 0; j < 3; j++) {
            vertData << pos[j].x() << pos[j].y() << pos[j].z()
                     << faceNorm.x() << faceNorm.y() << faceNorm.z()
                     << norm[j].x() << norm[j].y() << norm[j].z()
                     << c[0] << c[1] << c[2] << c[3];
        }
    }
    mVertexCount = triNum * 3;

    mTexture = new QOpenGLTexture(QImage(":/res/3d/maxim_top.png"));
    mTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    mTexture->setMagnificationFilter(QOpenGLTexture::Linear);

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
