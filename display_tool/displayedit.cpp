/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "displayedit.h"
#include <QDebug>
#include <cmath>

DisplayEdit::DisplayEdit(QWidget *parent) : QWidget(parent)
{
    mMouseLastX = 1000000;
    mMouseLastY = 1000000;
    mEditColor = Qt::white;
    mDrawLayer2 = true;

    setImageSize(128, 128);

    setMouseTracking(true);
}

QPoint DisplayEdit::getPixelUnderCursor()
{
    QPoint p = this->mapFromGlobal(QCursor::pos());
    int x = floor(((double)p.x() - mXOffset - (double)width() / 2.0) / mScaleFactor);
    int y = floor(((double)p.y() - mYOffset - (double)height() / 2.0) / mScaleFactor);
    return QPoint(x, y);
}

QImage DisplayEdit::getImageNow()
{
    QImage res(mPixelsX, mPixelsY, QImage::Format_RGB32);

    for (int i = 0;i < mPixelsX;i++) {
        for (int j = 0;j < mPixelsY;j++) {
            res.setPixelColor(i, j, pixel(i, j, false));
        }
    }

    return res;
}

QImage DisplayEdit::getImageBase()
{
    QImage res(mPixelsX, mPixelsY, QImage::Format_RGB32);

    for (int i = 0;i < mPixelsX;i++) {
        for (int j = 0;j < mPixelsY;j++) {
            res.setPixelColor(i, j, mImage.at(i).at(j));
        }
    }

    return res;
}

void DisplayEdit::loadFromImage(QImage img)
{
    for (int i = 0;i < mPixelsX;i++) {
        for (int j = 0;j < mPixelsY;j++) {
            QColor c = img.pixelColor(i, j);
            QColor cM = mMaskImage.at(i).at(j);

            if (cM == Qt::black && c.isValid() && c.alpha() > 0.0) {
                mImage[i][j] = c;
            }
        }
    }

    if (checkChanged()) {
        update();
    }
}

void DisplayEdit::loadMaskImage(QImage img)
{
    for (int i = 0;i < mPixelsX;i++) {
        for (int j = 0;j < mPixelsY;j++) {
            QColor c = img.pixelColor(i, j);

            if (c.isValid() && c.alpha() > 0.0 && c != Qt::black) {
                mImage[i][j] = Qt::black;
                mMaskImage[i][j] = c;
            }
        }
    }
}

void DisplayEdit::setImageSize(int width, int height)
{
    mPixelsX = width;
    mPixelsY = height;
    mScaleFactor = 8.0;
    mXOffset = -mPixelsX * 4;
    mYOffset = -mPixelsY * 4;

    mImage.resize(mPixelsX);

    for (auto &v: mImage) {
        v.resize(mPixelsY);
        for (auto &p: v) {
            p = Qt::black;
        }
    }

    mLastImage = mImage;
    mMaskImage = mImage;
    mOverlayImage = mImage;
    mLayer2Image = mImage;

    for (auto &v: mLayer2Image) {
        for (auto &p: v) {
            p.setAlpha(0);
        }
    }

    for (auto &v: mOverlayImage) {
        for (auto &p: v) {
            p.setAlpha(0);
        }
    }
}

QSize DisplayEdit::getImageSize()
{
    return QSize(mPixelsX, mPixelsY);
}

QColor DisplayEdit::getEditColor() const
{
    return mEditColor;
}

void DisplayEdit::setEditColor(const QColor &editColor)
{
    mEditColor = editColor;
    emit editColorChanged(mEditColor);
}

void DisplayEdit::setOverlayImage(
        int x, int y, // Where on display
        int x_start, int x_width, int y_start, int y_width, // Window to draw in (crop)
        int cx, int cy, // Center pixel of image
        float xr, float yr, // Pixel to rotate around
        float rot, // Rotation angle in degrees
        float scale, // Scale factor
        QColor transparent_color, // Color to be drawn as transparent
        QImage image)
{
    float sr = sinf(-rot * M_PI / 180.0);
    float cr = cosf(-rot * M_PI / 180.0);

    x -= cx * scale;
    y -= cy * scale;

    xr *= scale;
    yr *= scale;

    int x_end = (x_start + x_width);
    int y_end = (y_start + y_width);

    if (x_start < 0) {
        x_start = 0;
    }

    if (x_end > mPixelsX) {
        x_end = mPixelsX;
    }

    if (y_start < 0) {
        y_start = 0;
    }

    if (y_end > mPixelsY) {
        y_end = mPixelsY;
    }

    for (int j = y_start;j < y_end;j++) {
        for (int i = x_start;i < x_end;i++) {
            int px = (float)(i - x - xr) * cr + (float)(j - y - yr) * sr;
            int py = -(float)(i - x - xr) * sr + (float)(j - y - yr) * cr;

            px += xr;
            py += yr;

            px /= scale;
            py /= scale;

            if (px >= 0 && px < image.width() && py >= 0 && py < image.height()) {
                QColor pix = image.pixelColor(px, py);
                if (pix != transparent_color) {
                    mOverlayImage[i][j] = image.pixelColor(px, py);
                }
            }
        }
    }

    update();
    emit imageUpdated(getImageNow());
}

void DisplayEdit::clearOverlayImage()
{
    for (auto &v: mOverlayImage) {
        for (auto &p: v) {
            p = Qt::black;
            p.setAlpha(0);
        }
    }

    update();
    emit imageUpdated(getImageNow());
}

void DisplayEdit::saveOverlayToLayer2()
{
    for (int i = 0;i < mPixelsX;i++) {
        for (int j = 0;j < mPixelsY;j++) {
            QColor cO = mOverlayImage.at(i).at(j);

            if (cO.alpha() != 0) {
                mLayer2Image[i][j] = cO;
            }
        }
    }
}

void DisplayEdit::clearImage()
{
    for (auto &v: mImage) {
        for (auto &p: v) {
            p = Qt::black;
        }
    }

    update();
    checkChanged();
}

void DisplayEdit::clearLayer2()
{
    for (auto &v: mLayer2Image) {
        for (auto &p: v) {
            p.setAlpha(0);
        }
    }

    update();
    emit imageUpdated(getImageNow());
}

bool DisplayEdit::getDrawLayer2() const
{
    return mDrawLayer2;
}

void DisplayEdit::setDrawLayer2(bool drawLayer2)
{
    mDrawLayer2 = drawLayer2;
    update();
    emit imageUpdated(getImageNow());
}

void DisplayEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    int width = event->rect().width();
    int height = event->rect().height();

    bool antialiasDrawings = false;

    painter.setRenderHint(QPainter::Antialiasing, antialiasDrawings);
    painter.setRenderHint(QPainter::TextAntialiasing, antialiasDrawings);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, antialiasDrawings);

    const double scaleMax = 40;
    const double scaleMin = 0.5;

    // Make sure scale and offset is reasonable
    if (mScaleFactor < scaleMin) {
        double scaleDiff = scaleMin / mScaleFactor;
        mScaleFactor = scaleMin;
        mXOffset *= scaleDiff;
        mYOffset *= scaleDiff;
    } else if (mScaleFactor > scaleMax) {
        double scaleDiff = scaleMax / mScaleFactor;
        mScaleFactor = scaleMax;
        mXOffset *= scaleDiff;
        mYOffset *= scaleDiff;
    }

    // Limit the offset to avoid overflow at 2^31 mm
    double lim = 2000000000.0 * mScaleFactor;
    if (mXOffset > lim) {
        mXOffset = lim;
    } else if (mXOffset < -lim) {
        mXOffset = -lim;
    }

    if (mYOffset > lim) {
        mYOffset = lim;
    } else if (mYOffset < -lim) {
        mYOffset = -lim;
    }

    painter.fillRect(0, 0, width, height, QBrush(Qt::black));

    QFont font = this->font();

    // Map coordinate transforms
    QTransform drawTrans;
    drawTrans.translate(width / 2 + mXOffset, height / 2 + mYOffset);
    drawTrans.scale(mScaleFactor, mScaleFactor);

    // Text coordinates
    QTransform txtTrans;
    txtTrans.translate(0, 0);
    txtTrans.scale(1, 1);
    txtTrans.rotate(0);

    font.setPointSize(10);
    font.setFamily("Monospace");
    painter.setFont(font);

    painter.setTransform(drawTrans);

    QPoint px = getPixelUnderCursor();

    for (int i = 0;i < mPixelsX;i++) {
        for (int j = 0;j < mPixelsY;j++) {
            QColor cM = mMaskImage.at(i).at(j);
            QColor cPix = pixel(i, j, true);

            if (cM == Qt::black && i == px.x() && j == px.y()) {
                if (cPix.greenF() < 0.5) {
                    cPix.setGreenF(cPix.greenF() + 0.2);
                } else {
                    cPix.setRedF(cPix.redF() - 0.2);
                    cPix.setBlueF(cPix.blueF() - 0.2);
                }
            }

            painter.fillRect(i, j, 1.0, 1.0, cPix);
        }
    }

    painter.setTransform(txtTrans);

    QPen pen;
    pen.setColor(Qt::darkGray);
    pen.setWidthF(0.5);
    painter.setPen(pen);

    for (int i = 0;i <= mPixelsY;i++) {
        QPointF p1(0, i), p2(mPixelsX, i);
        QPointF p1_2 = drawTrans.map(p1);
        QPointF p2_2 = drawTrans.map(p2);
        painter.drawLine(p1_2, p2_2);
    }

    for (int i = 0;i <= mPixelsX;i++) {
        QPointF p1(i, 0), p2(i, mPixelsY);
        QPointF p1_2 = drawTrans.map(p1);
        QPointF p2_2 = drawTrans.map(p2);
        painter.drawLine(p1_2, p2_2);
    }

    painter.setTransform(txtTrans);
    painter.setOpacity(0.5);
    painter.fillRect(width - 100, height - 50, 100, 50, Qt::black);
    painter.setOpacity(1.0);
    painter.setPen(Qt::white);
    painter.drawText(width - 90, height - 50, 90, 50, Qt::AlignVCenter | Qt::AlignLeft,
                     QString("X: %1\nY: %2").arg(px.x()).arg(px.y()));
}

void DisplayEdit::mouseMoveEvent(QMouseEvent *e)
{
    bool ctrl = e->modifiers() == Qt::ControlModifier;
    bool shift = e->modifiers() == Qt::ShiftModifier;
    bool ctrl_shift = e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier);

    bool isLb = e->buttons() & Qt::LeftButton;

    if (e->buttons() & Qt::LeftButton && !ctrl && !shift && !ctrl_shift) {
        int x = e->pos().x();
        int y = e->pos().y();

        if (mMouseLastX < 100000) {
            int diffx = x - mMouseLastX;
            mXOffset += diffx;
        }

        if (mMouseLastY < 100000) {
            int diffy = y - mMouseLastY;
            mYOffset += diffy;
        }

        mMouseLastX = x;
        mMouseLastY = y;
    } else if (isLb && !ctrl && shift && !ctrl_shift) {
        QPoint px = getPixelUnderCursor();
        int x = px.x();
        int y = px.y();
        if (x >= 0 && x < mPixelsX && y >= 0 && y < mPixelsY) {
            if (mMaskImage.at(x).at(y) == Qt::black) {
                mImage[x][y] = mEditColor;
                checkChanged();
            }
        }
    }

    update();
}

void DisplayEdit::mousePressEvent(QMouseEvent *e)
{
    setFocus();

    bool ctrl = e->modifiers() == Qt::ControlModifier;
    bool shift = e->modifiers() == Qt::ShiftModifier;
    bool ctrl_shift = e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier);

    bool isLb = e->buttons() & Qt::LeftButton;
    bool isRb = e->buttons() & Qt::RightButton;

    if (ctrl) {
        if (e->buttons() & Qt::LeftButton) {

        } else if (e->buttons() & Qt::RightButton) {

        }
    } else if (shift) {
        QPoint px = getPixelUnderCursor();
        int x = px.x();
        int y = px.y();
        if (x >= 0 && x < mPixelsX && y >= 0 && y < mPixelsY) {
            if (mMaskImage.at(x).at(y) == Qt::black) {
                if (isLb) {
                    mImage[x][y] = mEditColor;
                    if (checkChanged()) {
                        update();
                    }
                } else if (isRb) {
                    mEditColor = mImage[x][y];
                    emit editColorChanged(mEditColor);
                }
            }
        }
    } else if (ctrl_shift) {
        if (e->buttons() & Qt::LeftButton) {

        } else if (e->buttons() & Qt::RightButton) {

        }
    }
}

void DisplayEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if (!(e->buttons() & Qt::LeftButton)) {
        mMouseLastX = 1000000;
        mMouseLastY = 1000000;
    }
}

void DisplayEdit::wheelEvent(QWheelEvent *e)
{
    bool ctrl = e->modifiers() == Qt::ControlModifier;
    bool shift = e->modifiers() == Qt::ShiftModifier;
    bool ctrl_shift = e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier);

    if (ctrl || shift || ctrl_shift) {

    } else {
        double scaleDiff = ((double)e->angleDelta().y() / 600.0);
        if (scaleDiff > 0.8) {
            scaleDiff = 0.8;
        }

        if (scaleDiff < -0.8) {
            scaleDiff = -0.8;
        }

        mScaleFactor += mScaleFactor * scaleDiff;
        mXOffset += mXOffset * scaleDiff;
        mYOffset += mYOffset * scaleDiff;
        update();
    }
}

bool DisplayEdit::checkChanged()
{
    bool changed = false;

    if (mImage != mLastImage) {
        mLastImage = mImage;
        changed = true;
    }

    if (changed) {
        emit imageUpdated(getImageNow());
    }

    return changed;
}

QColor DisplayEdit::pixel(int x, int y, bool withMask)
{
    QColor c = mImage.at(x).at(y);
    QColor cM = mMaskImage.at(x).at(y);
    QColor cO = mOverlayImage.at(x).at(y);
    QColor cL2 = mLayer2Image.at(x).at(y);

    QColor cPix = Qt::black;
    if (cM == Qt::black) {
        if (cO.alpha() == 0) {
            if (!mDrawLayer2 || cL2.alpha() == 0) {
                cPix = c;
            } else {
                cPix = cL2;
            }
        } else {
            cPix.setRedF(cO.redF() * cO.alphaF() + c.redF() * (1.0 - cO.alphaF()));
            cPix.setGreenF(cO.greenF() * cO.alphaF() + c.greenF() * (1.0 - cO.alphaF()));
            cPix.setBlueF(cO.blueF() * cO.alphaF() + c.blueF() * (1.0 - cO.alphaF()));
        }
    } else {
        cPix = withMask ? cM : Qt::black;
    }

    return cPix;
}
