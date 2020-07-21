////////////////////////////////////////////////////////////////////////////////
///	\file   usermapslayer.h
///
///	\author Elreg
///
///	\brief	Declaration of the CUserMapsLayer class which represents
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#ifndef CUSERMAPSLAYER_H
#define CUSERMAPSLAYER_H

#include "usermapslayerlib_global.h"
#include "../LayerLib/baselayer.h"
#include "../NavUtilsLib/coordinates.h"
#include "usermapsmanager.h"
#include "userpointpositiontype.h"
#include <QTimer>

class USERMAPSLAYERLIB_API CUserMapsLayer : public CBaseLayer
{
    Q_OBJECT

public:
    CUserMapsLayer(QQuickItem *parent = nullptr);
    ~CUserMapsLayer();

    QQuickFramebufferObject::Renderer* createRenderer() const override;

public slots:
    void setSelectedObject(bool isObjSelected, EUserMapObjectType objType);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void pressTimerTimeout();

private:
    // Coordinate conversion functions
    static void convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints);
    static void convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints);
    static void convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector, QVector<CPosition> &geoPoints);
    static CPosition convertPixelPointToGeoPoint(const QPointF &pixelPoint);
    static QPointF convertGeoPointToPixelPoint(const CPosition &geoPoint);

    // Object manipulation
    void onPositionClicked(const QPointF &clickedPosition);
    void updateObjectPosition();

    // Handling press/move events
    // if long mouse press at specific point (for line/area object) - deleting point
    // if long mouse press on line (for line/area object) - adding point
    // if mouse press on line (for line/area object) - moving line points???
    // if mouse press on line (for circle object) - resize circle = change radius
    // if mouse press inside object - moving object
    // if mouse press outside object - no action
    void handleObjAction(const QPointF &clickedPosition);
    void handleAreaObj(const QPointF &clickedPosition);
    void handleLineObj(const QPointF &clickedPosition);
    void handleCircleObj(const QPointF &clickedPosition);
    void handlePointObj(const QPointF &clickedPosition);
    void moveObj(void);
    void moveObjPoint(const int index);
    void moveObjPoints(const int index1, const int index2);
    void deleteObjPoint(const int index);
    void addObjPoint(const int index, const QPointF &clickedPoint);

    // Position estimation of clicked point
    EPointPositionType checkPointPosition (const QPointF &clickedPosition, int &index1, int &index2);
    EPointPositionType checkPointPosition (const QPointF &clickedPosition);
    EPointPositionType pointPositionToArea(const QPointF &clickedPosition, int &index1, int &index2);
    EPointPositionType pointPositionToCircle(const QPointF &clickedPosition);
    EPointPositionType pointPositionToLine(const QPointF &clickedPosition, int &index1, int &index2);
    qreal calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint);

    QTimer m_onPressTimer;                  ///< Timer for press event
    bool m_isCursorMoving;                  ///< Flag describing whether the cursor is moving or not.
    bool m_isLongMousePress;                  ///< Flag describing whether long mouse press occurred or not.
    QPointF m_moveEvtStartPoint;            ///< Move event start point in pixels
    EUserMapObjectType m_objectType;        ///< Type of object
    QVector<QPointF> m_selectedObjPoints;   ///< Vector of points of selected object
    qint64 m_moveEvtTimestamp;              ///< Timestamp for move event
};

#endif // CUSERMAPSLAYER_H
