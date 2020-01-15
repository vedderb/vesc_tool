/*
    Copyright 2012 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QBrush>
#include <QFont>
#include <QPen>
#include <QPalette>
#include <QList>
#include <QInputDialog>
#include <QTimer>
#include <QPinchGesture>
#include <QImage>
#include <QTransform>

#include "locpoint.h"
#include "carinfo.h"
#include "copterinfo.h"
#include "perspectivepixmap.h"
#include "osmclient.h"

class MapModule
{
public:
    virtual void processPaint(QPainter &painter, int width, int height, bool highQuality,
                              QTransform drawTrans, QTransform txtTrans, double scale) = 0;
    virtual bool processMouse(bool isPress, bool isRelease, bool isMove, bool isWheel,
                              QPoint widgetPos, LocPoint mapPos, double wheelAngleDelta,
                              bool ctrl, bool shift, bool ctrlShift,
                              bool leftButton, bool rightButton, double scale) = 0;
};

class MapWidget : public QWidget
{
    Q_OBJECT

public:
    typedef enum {
        InteractionModeDefault,
        InteractionModeCtrlDown,
        InteractionModeShiftDown,
        InteractionModeCtrlShiftDown
    } InteractionMode;

    explicit MapWidget(QWidget *parent = nullptr);
    CarInfo* getCarInfo(int car);
    CopterInfo* getCopterInfo(int copter);
    void setFollowCar(int car);
    void setTraceCar(int car);
    void setSelectedCar(int car);
    void addCar(const CarInfo &car);
    void addCopter(const CopterInfo &copter);
    bool removeCar(int carId);
    bool removeCopter(int copterId);
    void clearCars();
    void clearCopters();
    LocPoint* getAnchor(int id);
    void addAnchor(const LocPoint &anchor);
    bool removeAnchor(int id);
    void clearAnchors();
    QList<LocPoint> getAnchors();
    void setScaleFactor(double scale);
    double getScaleFactor();
    void setRotation(double rotation);
    void setXOffset(double offset);
    void setYOffset(double offset);
    void moveView(double px, double py);
    void clearTrace();
    void addRoutePoint(double px, double py, double speed = 0.0, qint32 time = 0);
    QList<LocPoint> getRoute(int ind = -1);
    QList<QList<LocPoint> > getRoutes();
    void setRoute(const QList<LocPoint> &route);
    void addRoute(const QList<LocPoint> &route);
    int getRouteNum();
    void clearRoute();
    void clearAllRoutes();
    void setRoutePointSpeed(double speed);
    void addInfoPoint(LocPoint &info, bool updateMap = true);
    void clearInfoTrace();
    void clearAllInfoTraces();
    void addPerspectivePixmap(PerspectivePixmap map);
    void clearPerspectivePixmaps();
    QPoint getMousePosRelative();
    void setAntialiasDrawings(bool antialias);
    void setAntialiasOsm(bool antialias);
    bool getDrawOpenStreetmap() const;
    void setDrawOpenStreetmap(bool drawOpenStreetmap);
    void setEnuRef(double lat, double lon, double height);
    void getEnuRef(double *llh);
    double getOsmRes() const;
    void setOsmRes(double osmRes);
    double getInfoTraceTextZoom() const;
    void setInfoTraceTextZoom(double infoTraceTextZoom);
    OsmClient *osmClient();
    int getInfoTraceNum();
    int getInfoPointsInTrace(int trace);
    int setNextEmptyOrCreateNewInfoTrace();
    void setAnchorMode(bool anchorMode);
    bool getAnchorMode();
    void setAnchorId(int id);
    void setAnchorHeight(double height);
    void removeLastRoutePoint();
    void zoomInOnRoute(int id, double margins, double wWidth = -1, double wHeight = -1);
    void zoomInOnInfoTrace(int id, double margins, double wWidth = -1, double wHeight = -1);

    int getOsmMaxZoomLevel() const;
    void setOsmMaxZoomLevel(int osmMaxZoomLevel);

    int getOsmZoomLevel() const;

    bool getDrawGrid() const;
    void setDrawGrid(bool drawGrid);

    bool getDrawOsmStats() const;
    void setDrawOsmStats(bool drawOsmStats);

    int getRouteNow() const;
    void setRouteNow(int routeNow);

    qint32 getRoutePointTime() const;
    void setRoutePointTime(const qint32 &routePointTime);

    double getTraceMinSpaceCar() const;
    void setTraceMinSpaceCar(double traceMinSpaceCar);

    double getTraceMinSpaceGps() const;
    void setTraceMinSpaceGps(double traceMinSpaceGps);

    int getInfoTraceNow() const;
    void setInfoTraceNow(int infoTraceNow);

    void printPdf(QString path, int width = 0, int height = 0);
    void printPng(QString path, int width = 0, int height = 0);

    bool getDrawRouteText() const;
    void setDrawRouteText(bool drawRouteText);

    bool getDrawUwbTrace() const;
    void setDrawUwbTrace(bool drawUwbTrace);

    void setLastCameraImage(const QImage &lastCameraImage);

    double getCameraImageWidth() const;
    void setCameraImageWidth(double cameraImageWidth);

    double getCameraImageOpacity() const;
    void setCameraImageOpacity(double cameraImageOpacity);

    MapWidget::InteractionMode getInteractionMode() const;
    void setInteractionMode(const MapWidget::InteractionMode &controlMode);

    void addMapModule(MapModule *m);
    void removeMapModule(MapModule *m);
    void removeMapModuleLast();


signals:
    void scaleChanged(double newScale);
    void offsetChanged(double newXOffset, double newYOffset);
    void posSet(int id, LocPoint pos);
    void routePointAdded(LocPoint pos);
    void lastRoutePointRemoved(LocPoint pos);
    void infoTraceChanged(int traceNow);
    void infoPointClicked(LocPoint info);

private slots:
    void tileReady(OsmTile tile);
    void errorGetTile(QString reason);
    void timerSlot();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent (QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    bool event(QEvent *event) override;

private:
    QList<CarInfo> mCarInfo;
    QList<CopterInfo> mCopterInfo;
    QVector<LocPoint> mCarTrace;
    QVector<LocPoint> mCarTraceGps;
    QVector<LocPoint> mCarTraceUwb;
    QList<LocPoint> mAnchors;
    QList<QList<LocPoint> > mRoutes;
    QList<QList<LocPoint> > mInfoTraces;
    QList<LocPoint> mVisibleInfoTracePoints;
    QList<PerspectivePixmap> mPerspectivePixmaps;
    double mRoutePointSpeed;
    qint32 mRoutePointTime;
    qint32 mAnchorId;
    double mAnchorHeight;
    double mScaleFactor;
    double mRotation;
    double mXOffset;
    double mYOffset;
    int mMouseLastX;
    int mMouseLastY;
    int mFollowCar;
    int mTraceCar;
    int mSelectedCar;
    double xRealPos;
    double yRealPos;
    bool mAntialiasDrawings;
    bool mAntialiasOsm;
    double mOsmRes;
    double mInfoTraceTextZoom;
    OsmClient *mOsm;
    int mOsmZoomLevel;
    int mOsmMaxZoomLevel;
    bool mDrawOpenStreetmap;
    bool mDrawOsmStats;
    double mRefLat;
    double mRefLon;
    double mRefHeight;
    LocPoint mClosestInfo;
    bool mDrawGrid;
    int mRoutePointSelected;
    int mAnchorSelected;
    int mRouteNow;
    int mInfoTraceNow;
    double mTraceMinSpaceCar;
    double mTraceMinSpaceGps;
    QList<QPixmap> mPixmaps;
    bool mAnchorMode;
    bool mDrawRouteText;
    bool mDrawUwbTrace;
    QImage mLastCameraImage;
    double mCameraImageWidth;
    double mCameraImageOpacity;
    InteractionMode mInteractionMode;
    QTimer *mTimer;
    QVector<MapModule*> mMapModules;

    void updateClosestInfoPoint();
    int drawInfoPoints(QPainter &painter, const QList<LocPoint> &pts,
                        QTransform drawTrans, QTransform txtTrans,
                       double xStart, double xEnd, double yStart, double yEnd,
                       double min_dist);
    int getClosestPoint(LocPoint p, QList<LocPoint> points, double &dist);
    void drawCircleFast(QPainter &painter, QPointF center, double radius, int type = 0);

    void paint(QPainter &painter, int width, int height, bool highQuality = false);
    void updateTraces();
};

#endif // MAPWIDGET_H
