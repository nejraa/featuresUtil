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
    EPointPositionType checkPointPosition (const QPointF &clickedPosition, int &index1, int &index2);

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

    // Position estimation of clicked point
    EPointPositionType pointPositionToArea(const QPointF &clickedPosition, int &index1, int &index2);
    EPointPositionType pointPositionToCircle(const QPointF &clickedPosition);
    EPointPositionType pointPositionToLine(const QPointF &clickedPosition, int &index1, int &index2);
    qreal calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint);

    QTimer m_onPressTimer;                  ///< Timer for press event
    bool m_isCursorMoving;                  ///< Flag describing whether the cursor is moving or not.
    QPointF m_moveEvtStartPoint;            ///< Move event start point in pixels
    EUserMapObjectType m_objectType;        ///< Type of object
    QVector<QPointF> m_selectedObjPoints;   ///< Vector of points of selected object
    qint64 m_moveEvtTimestamp;              ///< Timestamp for move event
};

#endif // CUSERMAPSLAYER_H
