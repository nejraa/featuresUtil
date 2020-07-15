////////////////////////////////////////////////////////////////////////////////
///	\file   usermapslayer.cpp
///
///	\author Elreg
///
///	\brief	Definition of the CUserMapsLayer class which represents
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#include "usermapslayer.h"
#include "usermapsrenderer.h"
#include <QDebug>
#include <QtMath>
#include <QSharedPointer>
#include "../NavUtilsLib/coordinates.h"
#include <QDateTime>
#include "usermapsmanager.h"

#define PIXEL_OFFSET 10
const int MOVE_EVT_TIME_LIMIT = 500;
const int LONG_PRESS_DURATION_MS = 2000;

CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
    : CBaseLayer(parent)
    , m_moveEvtTimestamp(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    QObject::connect(CUserMapsManager::instance(), &CUserMapsManager::selectedObjChanged,
                     this, &CUserMapsLayer::setSelectedObject);

    m_onPressTimer.setSingleShot(true);
    m_onPressTimer.setInterval(LONG_PRESS_DURATION_MS);
    connect(&m_onPressTimer, &QTimer::timeout, this, &CUserMapsLayer::pressTimerTimeout);
}

CUserMapsLayer::~CUserMapsLayer()
{

}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::mousePressEvent(QMouseEvent *event)
///
/// \brief      When user press on the UI.
///
/// \param		event Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "PRESSED";
    //if (! CUserMapsManager::getEditModeOn())
    //    return;

    m_onPressTimer.start();
    m_moveEvtStartPoint = event->screenPos();
    m_isCursorMoving = false;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
///
/// \brief      When user press and moves on UI.
///
/// \param		event Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
{
    //if (! CUserMapsManager::getEditModeOn())
    //    return;

    const qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    if(currentTimestamp - m_moveEvtTimestamp < MOVE_EVT_TIME_LIMIT)
        return;

    if(m_onPressTimer.isActive() || m_isCursorMoving)
    {
        m_onPressTimer.stop();
        m_isCursorMoving = true;
        m_moveEvtTimestamp = currentTimestamp;

        QPointF pointDifference = m_moveEvtStartPoint - event->screenPos();
        m_moveEvtStartPoint = event->screenPos();
        for (int i = 0; i < m_selectedObjPoints.size(); i++)
            m_selectedObjPoints[i] -= pointDifference;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
///
/// \brief      When user release.
///
/// \param		event Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
{
    //if (! CUserMapsManager::getEditModeOn())
    //    return;

    if(m_onPressTimer.isActive() && !m_isCursorMoving)
        onPositionClicked(event->screenPos());

    updateObjectPosition();
    m_isCursorMoving = false;
    m_onPressTimer.stop();
}

void CUserMapsLayer::pressTimerTimeout()
{
    if(!m_isCursorMoving)
        qDebug() << "Long press";
}

////////////////////////////////////////////////////////////////////////////////
/// \fn		QQuickFramebufferObject::Renderer* CUserMapsLayer::createRenderer() const
///
/// \brief	Creates the renderer that's associated with user maps layer.
///
/// \return Pointer to the renderer.
////////////////////////////////////////////////////////////////////////////////
QQuickFramebufferObject::Renderer *CUserMapsLayer::createRenderer() const
{
    return new CUserMapsRenderer();
}

////////////////////////////////////////////////////////////////////////////////
/// \fn		EPointPositionType CUserMapsLayer::checkPointPosition(const QPointF &clickedPosition)
///
/// \brief	Checks whether clicked point lies inside selected object boundaries.
///
/// \param  clickedPoint Clicked point.
///
/// \return EPointPositionType Position of clicked point with respect to an object.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::checkPointPosition(const QPointF &clickedPosition)
{
    switch (m_objectType)
        {
        case EUserMapObjectType::Area:
            return pointPositionToArea(clickedPosition);
        case EUserMapObjectType::Circle:
            return pointPositionToCircle(clickedPosition);
        case EUserMapObjectType::Line:
            return pointPositionToLine(clickedPosition);
        case EUserMapObjectType::Point:
            return EPointPositionType::InsideObject;
        case EUserMapObjectType::Unkown_Object:
            return EPointPositionType::Unknown;
        }

}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::selectedObjType(EUserMapObjectType objType)
///
/// \brief		Based on object type it gets the position points.
///
/// \param		objType Type of object.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::setSelectedObject(bool isObjSelected, EUserMapObjectType objType)
{
    if (! isObjSelected)
        return; //TODO: renderer needs this info.

    m_objectType = objType;
    switch (objType)
    {
    case EUserMapObjectType::Point:
    case EUserMapObjectType::Circle:
        //convertGeoPointToPixelVector(CUserMapsManager::getObjPosition(), m_selectedObjPoints);
        break;
    case EUserMapObjectType::Area:
    case EUserMapObjectType::Line:
        //convertGeoVectorToPixelVector(CUserMapsManager::getObjPointsVector(), m_selectedObjPoints);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn		CUserMapsLayer::convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoint,
///														  QVector<QPointF> &pixelPoints)
///
/// \brief	Converts vector of Geo coordinates into vector of pixel coordinates.
///
/// \param	geoPoints Geo coordinates.
/// \param  pixelPoints Pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints)
{
    //	CCoordinates c;
    pixelPoints.clear();
    pixelPoints.reserve(geoPoints.size());
    for (const CPosition &point : geoPoints)
    {
        //		pixelPoints.append(c.convertGeoCoordsToPixelCoords(point));

        //TODO
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::convertGeoPointToPixelVector(const CPosition &geoPoint,
///														     QVector<QPointF> &pixelPoints)
/// \brief      Converts Geo point into pixel vector.
///
/// \param		geoPoint Geo coordinates.
/// \param		pixelPoints Pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints)
{
    //	CCoordinates c;
    pixelPoints.clear();
    //	pixelPoints.append(c.convertGeoCoordsToPixelCoords(geoPoint));
    //TODO
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector)
///
/// \brief      Converts vector of pixel coordinates into vector of geo coordinates.
///
/// \param		pixelVector Vector of pixel coordinates.
///
/// \return		Vector with geo coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector, QVector<CPosition> &geoPoints)
{
    //	CCoordinates c;
    geoPoints.clear();
    geoPoints.reserve(pixelVector.size());
    for (const QPointF &pixelPoint : pixelVector)
    {
        //		GEOGRAPHICAL lat;
        //		GEOGRAPHICAL lon;

        //		//qDebug() << "!!!CCursorReadout::setMousePosition!!!";

        //		//qDebug() << "cursor position  m_pos_x: " << m_pos_x << "  m_pos_y: "<<  m_pos_y;
        //		CViewCoordinates::Instance()->Convert(PIXEL(point.x()), PIXEL(point.y()), lat, lon);

        //		geoPoints.append(CPosition(double(lat), double(lon)));

        geoPoints.append(convertPixelPointToGeoPoint(pixelPoint));
    }
}

CPosition CUserMapsLayer::convertPixelPointToGeoPoint(const QPointF &pixelPoint)
{
    GEOGRAPHICAL lat;
    GEOGRAPHICAL lon;
    CViewCoordinates::Instance()->Convert(PIXEL(pixelPoint.x()), PIXEL(pixelPoint.y()), lat, lon);
    return CPosition(double(lat), double(lon));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         QPointF CUserMapsLayer::convertGeoPointToPixelPoint(const CPosition &geoPoint)
///
/// \brief      Converts geo coordinate into pixel coordinate.
///
/// \param		geoPoint Point in geo coordinates.
///
/// \return		Point in pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
QPointF CUserMapsLayer::convertGeoPointToPixelPoint(const CPosition &geoPoint)
{
    double x = 0.0;
    double y = 0.0;
    // CViewCoordinates::Instance()->Convert(GEOGRAPHICAL(geoPoint.Latitude()), GEOGRAPHICAL(geoPoint.Longitude()), PIXEL(x), PIXEL(y) ));
    // TO DO - implementation of function above required
    return QPointF(x,y);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::onPositionClicked(const QPointF &clickedPosition)
///
/// \brief      When user clicks once.
///
/// \param		clickedPosition Position where user has clicked.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::onPositionClicked(const QPointF &clickedPosition)
{
    CPosition pos = convertPixelPointToGeoPoint(clickedPosition);

    if (CUserMapsManager::getCreatingNewObjStat())
    {
        CUserMapsManager::addObjPointStat(pos);
        return;
    }

    //if (CUserMapsManager::isObjSelected())
    //{
        //TODO: probably deselect on click?
    //}
    //else
    //{
    //    CUserMapsManager::selectObject(pos);
    //}
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::updateObjectPosition()
///
/// \brief      Updates object position based on his type.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::updateObjectPosition()
{
    if (m_selectedObjPoints.isEmpty())
        return;

    QVector<CPosition> geoPoints;
    convertPixelVectorToGeoVector(m_selectedObjPoints, geoPoints);

    switch (m_objectType)
    {
    case EUserMapObjectType::Point:
    case EUserMapObjectType::Circle:
        CUserMapsManager::setObjPositionStat(geoPoints[0]);
        break;
    case EUserMapObjectType::Area:
    case EUserMapObjectType::Line:
        CUserMapsManager::setObjPointsVectorStat(geoPoints);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         EPointPositionType CUserMapsLayer::pointPositionToArea(const QPointF &clickedPosition)
///
/// \brief      Returns position of clicked point with respect to an area object.
///
/// \param		clickedPosition Clicked position in pixel coordinates.
///
/// \return		EPointPositionType Selected point position with respect to an area.
EPointPositionType CUserMapsLayer::pointPositionToArea(const QPointF &clickedPosition)
{
    // TO DO
    // CUserMapArea vector of points (public CUserMapComplexObject) QVector<CPosition>

    m_selectedObjPoints;
    return EPointPositionType::InsideObject;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         EPointPositionType CUserMapsLayer::pointPositionToCircle(const QPointF &clickedPosition)
///
/// \brief      Returns position of clicked point with respect to a circle object.
///
/// \param		clickedPosition Clicked position in pixel coordinates.
///
/// \return		EPointPositionType Selected point position with respect to a circle.
EPointPositionType CUserMapsLayer::pointPositionToCircle(const QPointF &clickedPosition)
{
    float radiusVal = CUserMapsManager::getObjRadiusStat();
    CPosition centerPoint = CUserMapsManager::getObjPositionStat();
    QPointF circleCenterPixel = convertGeoPointToPixelPoint(centerPoint);

    qreal distance = qSqrt(qPow(clickedPosition.x() - circleCenterPixel.x(), 2) + qPow(clickedPosition.y() - circleCenterPixel.y(), 2));
    if (distance < radiusVal )
    {
        return EPointPositionType::InsideObject;
    }
    else if (distance > radiusVal)
    {
        return  EPointPositionType::OutsideObject;
    }
    //else if ((distance <= radiusVal + PIXEL_OFFSET) && (distance >= radiusVal - PIXEL_OFFSET) )
    else if (qAbs(distance - radiusVal) <= PIXEL_OFFSET)
    {
        return EPointPositionType::OnLine;
    }
    else
    {
        return EPointPositionType::Unknown;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         EPointPositionType CUserMapsLayer::pointPositionToLine(const QPointF &clickedPosition)
///
/// \brief      Returns position of clicked point with respect to a line object.
///
/// \param		clickedPosition Clicked position in pixel coordinates.
///
/// \return		EPointPositionType Selected point position with respect to a line.
EPointPositionType CUserMapsLayer::pointPositionToLine(const QPointF &clickedPosition)
{
    QPointF pointA;
    QPointF pointB;
    qreal linePoint;
    int objPointsIndex;

    for ( objPointsIndex = 0; objPointsIndex <= m_selectedObjPoints.size() - 1; objPointsIndex++ )
    {
        pointA = m_selectedObjPoints[objPointsIndex];
        pointB = m_selectedObjPoints[objPointsIndex+1];

        if (pointA.x() <= pointB.x())
        {
            // check whether selected point lies between pointA and pointB
            if ((clickedPosition.x() >= pointA.x()) && (clickedPosition.x() <= pointB.x()) )
            {
                if ( (clickedPosition.y() >= pointA.y() && clickedPosition.y() <= pointB.y()) )
                {
                    // clicked position lies between pointA and pointB
                    // clicked position lies between pointA and pointB
                    // check if clicked position lies on line
                    linePoint = calculateYaxisValueOnLine(pointA, pointB, clickedPosition);

                    // check if calculated point lies around selected line
                    if (qAbs(clickedPosition.y() - linePoint) <= PIXEL_OFFSET)
                    {
                        return EPointPositionType::OnLine;
                    }
                    else
                    {
                        return EPointPositionType::NotOnLine;
                    }
                }
            }

        }
        else if (pointA.x() >= pointB.x())
        {
                // check whether selected point lies between pointA and pointB
                if ((clickedPosition.x() >= pointB.x()) && (clickedPosition.x() <= pointA.x()) )
                {
                    if ( (clickedPosition.y() <= pointB.y() && clickedPosition.y() >= pointA.y()) )
                    {
                        // clicked position lies between pointA and pointB
                        // check if clicked position lies on line
                        linePoint = calculateYaxisValueOnLine(pointA, pointB, clickedPosition);

                        // check if calculated point lies around selected line
                        if (qAbs(clickedPosition.y() - linePoint) <= PIXEL_OFFSET)
                        {
                            return EPointPositionType::OnLine;
                        }
                        else
                        {
                            return EPointPositionType::NotOnLine;
                        }
                    }
                }
         }
    }
    return EPointPositionType::Unknown;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         qreal CUserMapsLayer::calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint)
///
/// \brief      Calculates yAxis point value for given XAxis point of linear function.
///
/// \param		pointA - first point used to determine linear function equation.
///             pointB - second point used to determine linear function equation.
///             clickedPoint Clicked point.
///
/// \return		qreal Value on YAxis calculated using linear function equation.
qreal CUserMapsLayer::calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint)
{
    // line equation between two points
    // m = (yB − yA)/(xB − xA)
    // y = m (x − xA) + yA
    qreal m = (pointB.y() - pointA.y())/(pointB.x() - pointA.x());
    return m * (clickedPoint.x() - pointA.x()) + pointA.y();
}


