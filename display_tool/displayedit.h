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

#ifndef DISPLAYEDIT_H
#define DISPLAYEDIT_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>

class DisplayEdit : public QWidget
{
    Q_OBJECT
public:
    explicit DisplayEdit(QWidget *parent = nullptr);
    QPoint getPixelUnderCursor();
    QImage getImageNow();
    QImage getImageBase();
    void loadFromImage(QImage img);
    void loadMaskImage(QImage img);
    void setImageSize(int width, int height);
    QSize getImageSize();

    QColor getEditColor() const;
    void setEditColor(const QColor &editColor);

    void setOverlayImage(
            int x, int y, // Where on display
            int x_start, int x_width, int y_start, int y_width, // Window to draw in (crop)
            int cx, int cy, // Center pixel of image
            float xr, float yr, // Pixel to rotate around
            float rot, // Rotation angle in degrees
            float scale, // Scale factor
            QColor transparent_color, // Color to be drawn as transparane
            QImage image);
    void clearOverlayImage();
    void saveOverlayToLayer2();

    void clearImage();
    void clearLayer2();

    bool getDrawLayer2() const;
    void setDrawLayer2(bool drawLayer2);

signals:
    void imageUpdated(QImage img);
    void editColorChanged(QColor c);

public slots:

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent (QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    double mScaleFactor;
    double mXOffset;
    double mYOffset;
    int mMouseLastX;
    int mMouseLastY;
    int mPixelsX;
    int mPixelsY;
    QVector<QVector<QColor> > mMaskImage;
    QVector<QVector<QColor> > mOverlayImage;
    QVector<QVector<QColor> > mImage;
    QVector<QVector<QColor> > mLastImage;
    QVector<QVector<QColor> > mLayer2Image;
    QColor mEditColor;
    bool mDrawLayer2;

    bool checkChanged();
    QColor pixel(int x, int y, bool withMask);

};

#endif // DISPLAYEDIT_H
