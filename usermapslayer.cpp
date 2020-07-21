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

#include "usermaparea.h"
#include "usermapline.h"
#include "usermapcircle.h"

#define PIXEL_OFFSET 10
const int MOVE_EVT_TIME_LIMIT = 500;
const int LONG_PRESS_DURATION_MS = 2000;

////////////////////////////////////////////////////////////////////////////////
/// \fn   CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
///
/// \brief Constructor.
///
/// \param parent - QQUickItem to be used as parent.
/// \param m_moveEvtTimestamp - Object which the editor will operate on.
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
/// \fn   CUserMapsLayer::~CUserMapsLayer()
///
/// \brief Destructor.
////////////////////////////////////////////////////////////////////////////////
CUserMapsLayer::~CUserMapsLayer()
{

}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::mousePressEvent(QMouseEvent *event)
///
/// \brief      When user press on the UI.
///
/// \param		event - Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "PRESSED";
    //if (! CUserMapsManager::getEditModeOnStat())
      //  return;
    m_onPressTimer.start();
    m_moveEvtStartPoint = event->screenPos();
    m_isCursorMoving = false;

    /* // added for testing purposes
    int returnedIndex1, returnedindex2;
    EPointPositionType mltz = pointPositionToCircle(m_moveEvtStartPoint);
    EPointPositionType mltz = pointPositionToArea(m_moveEvtStartPoint, returnedIndex1, returnedindex2);
    EPointPositionType mltz = pointPositionToLine(m_moveEvtStartPoint, returnedIndex1, returnedindex2);
    qDebug() << "POSITION ESTIMATION"; */
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
///
/// \brief      When user press and moves on UI.
///
/// \param		event - Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
{
    if (! CUserMapsManager::getEditModeOnStat())
        return;

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
/// \param		event - Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
{
    if (! CUserMapsManager::getEditModeOnStat())
        return;

    if(m_onPressTimer.isActive() && !m_isCursorMoving)
        onPositionClicked(event->screenPos());

    updateObjectPosition();
    m_isCursorMoving = false;
    m_onPressTimer.stop();
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         void CUserMapsLayer::pressTimerTimeout()
///
/// \brief      Gives timer timeout for long press.
////////////////////////////////////////////////////////////////////////////////
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
/// \fn		EPointPositionType CUserMapsLayer::checkPointPosition(const QPointF &clickedPosition, int &index1, int &index2)
///
/// \brief	Checks whether clicked point lies inside selected object boundaries.
///
/// \param  clickedPoint - Clicked point.
/// \param  index1 - Index of the first point on line segment of area/line object where clicked position lies.
/// \param  index2 - Index of the second point on line segment of area/line object where clicked position lies.
///
/// \return EPointPositionType Position of clicked point with respect to an object.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::checkPointPosition(const QPointF &clickedPosition, int &index1, int &index2)
{
    switch (m_objectType)
        {
        case EUserMapObjectType::Area:
            return pointPositionToArea(clickedPosition, index1, index2);
        case EUserMapObjectType::Circle:
            return pointPositionToCircle(clickedPosition);
        case EUserMapObjectType::Line:
            return pointPositionToLine(clickedPosition, index1, index2);
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
/// \param		objType - Type of object.
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
        // convertGeoPointToPixelVector(CUserMapsManager::getObjPosition(), m_selectedObjPoints);
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
/// \param	geoPoints - Geo coordinates.
/// \param  pixelPoints - Pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints)
{
    pixelPoints.clear();
    pixelPoints.reserve(geoPoints.size());
    for (const CPosition &geoPoint : geoPoints)
    {
        pixelPoints.append(convertGeoPointToPixelPoint(geoPoint));
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::convertGeoPointToPixelVector(const CPosition &geoPoint,
///														     QVector<QPointF> &pixelPoints)
/// \brief      Converts Geo point into pixel vector.
///
/// \param		geoPoint - Geo coordinates.
/// \param  	pixelPoints - Pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints)
{
    pixelPoints.clear();
    pixelPoints.reserve(1);
    pixelPoints.append(convertGeoPointToPixelPoint(geoPoint));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector)
///
/// \brief      Converts vector of pixel coordinates into vector of geo coordinates.
///
/// \param		pixelVector - Vector of pixel coordinates.
///
/// \return		Vector with geo coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector, QVector<CPosition> &geoPoints)
{
    geoPoints.clear();
    geoPoints.reserve(pixelVector.size());
    for (const QPointF &pixelPoint : pixelVector)
    {
        geoPoints.append(convertPixelPointToGeoPoint(pixelPoint));
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CPosition CUserMapsLayer::convertPixelPointToGeoPoint(const QPointF &pixelPoint)
///
/// \brief      Converts pixel point into geo point.
///
/// \param		pixelPoint - Pixel coordinate to be converted to geo coordinate.
///
/// \return		Geo coordinate.
////////////////////////////////////////////////////////////////////////////////
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
/// \param		geoPoint - Point in geo coordinates.
///
/// \return		Point in pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
QPointF CUserMapsLayer::convertGeoPointToPixelPoint(const CPosition &geoPoint)
{
    PIXEL x = 0.0;
    PIXEL y = 0.0;
    CViewCoordinates::Instance()->Convert(GEOGRAPHICAL(geoPoint.Latitude()), GEOGRAPHICAL(geoPoint.Longitude()), x, y);
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
/// \fn         EPointPositionType CUserMapsLayer::pointPositionToArea(const QPointF &clickedPosition, int &index1, int &index2)
///
/// \brief      Returns position of clicked point with respect to an area object.
///
/// \param		clickedPosition - Clicked position in pixel coordinates.
/// \param      index1 - Index of the first point on area object between segments where clicked position lies.
/// \param      index2 - Index of the second point on area object between segments where clicked position lies.
///
/// \return		EPointPositionType Selected point position with respect to an area.
EPointPositionType CUserMapsLayer::pointPositionToArea(const QPointF &clickedPosition, int &index1, int &index2)
{
    /* // added for testing purposes
    QPointF pontTest1 = QPointF(800.0, 300.0);
    QPointF pontTest2 = QPointF(900.0, 300.0);
    QPointF pontTest3 = QPointF(900.0, 400.0);
    QPointF pontTest4 = QPointF(800.0, 400.0);
    QVector<QPointF> linePoints;
    linePoints.append(pontTest1);
    linePoints.append(pontTest2);
    linePoints.append(pontTest3);
    linePoints.append(pontTest4);
    linePoints.append(pontTest1);
    m_selectedObjPoints = linePoints;*/

    // detection of inner/outer position of selected point
    QPointF pointC;
    QPointF pointD;
    int crossingNum = 0;
    for (int i = 0; i < m_selectedObjPoints.size() - 1; i++)
    {
        pointC = m_selectedObjPoints[i];
        pointD = m_selectedObjPoints[i+1];

        if ( qAbs(clickedPosition.x() - pointC.x()) <= PIXEL_OFFSET &&
             qAbs(clickedPosition.y() - pointC.y()) <= PIXEL_OFFSET)
        {
            // clicked close to pointC
            // saves indices of pointC and pointD
            // (whereas the first saved index is an index of a point where clicked)
            index1 = i;
            index2 = i+1;
            return EPointPositionType::AtSpecificPoint;
        }
        else if ( qAbs(clickedPosition.x() - pointD.x()) <= PIXEL_OFFSET &&
                  qAbs(clickedPosition.y() - pointD.y()) <= PIXEL_OFFSET)
        {
            // clicked close to pointD
            // saves indices of pointD and pointC in index1 and index2
            // (whereas the first saved index is an index of a point where clicked)
            index1 = i+1;
            index2 = i;
            return EPointPositionType::AtSpecificPoint;
        }

        if (((pointC.y() <= clickedPosition.y() + PIXEL_OFFSET) && (pointD.y() + PIXEL_OFFSET > clickedPosition.y()))
            || ((pointC.y() + PIXEL_OFFSET > clickedPosition.y()) && (pointD.y() <=  clickedPosition.y() + PIXEL_OFFSET)))
        {
                qreal intersectX = (clickedPosition.y()  - pointC.y()) / (pointD.y() - pointC.y());
                if (clickedPosition.x() < pointC.x() + intersectX * (pointD.x() - pointC.x()))
                    ++crossingNum;
        }
    }

    if (crossingNum % 2 == 0)
    {
        index1 = -1;
        index2 = -1;
        return EPointPositionType::OutsideObject;
    }
    else
    {
        index1 = -1;
        index2 = -1;
        return EPointPositionType::InsideObject;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         EPointPositionType CUserMapsLayer::pointPositionToCircle(const QPointF &clickedPosition)
///
/// \brief      Returns position of clicked point with respect to a circle object.
///
/// \param		clickedPosition - Clicked position in pixel coordinates.
///
/// \return		EPointPositionType Selected point position with respect to a circle.
EPointPositionType CUserMapsLayer::pointPositionToCircle(const QPointF &clickedPosition)
{
    float radiusVal = CUserMapsManager::getObjRadiusStat();
    // radiusVal - radius of circle returned in nautical miles
    // conversion to pixel coordinates might be required
    // radiusVal = getNauticalMilesToPixels(radiusVal);
    CPosition centerPoint = CUserMapsManager::getObjPositionStat();
    QPointF circleCenterPixel = convertGeoPointToPixelPoint(centerPoint);

    /* // added for testing purposes
    float radiusVal = 100.0;
    QPointF circleCenterPixel = QPointF(200.0, 200.0);
    QVector<QPointF> linePoints1;
    linePoints1.insert(0, circleCenterPixel);
    QVector<QPointF> m_selectedObjPoints = linePoints1;*/

    qDebug() << "Radius" << radiusVal << "Center point: (" << circleCenterPixel.x() << "," << circleCenterPixel.y() << ")" ;
    qreal distance = qSqrt(qPow(clickedPosition.x() - circleCenterPixel.x(), 2) + qPow(clickedPosition.y() - circleCenterPixel.y(), 2));
    if (qAbs(distance - radiusVal) <= PIXEL_OFFSET)
    {
            return EPointPositionType::OnLine;
    }
    else if (distance > radiusVal)
    {
        return  EPointPositionType::OutsideObject;
    }
    else if ( distance < radiusVal )
    {
        return EPointPositionType::InsideObject;
    }
    else
    {
        return EPointPositionType::Unknown;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         EPointPositionType CUserMapsLayer::pointPositionToLine(const QPointF &clickedPosition, int &index1, int &index2)
///
/// \brief      Returns position of clicked point with respect to a line object.
///
/// \param		clickedPosition - Clicked position in pixel coordinates.
/// \param      pointC - Index of the first point on line object on segment where clicked position lies.
/// \param      pointD - Index of the second point on line object on segment where clicked position lies.
///
/// \return		EPointPositionType Selected point position with respect to a line.
EPointPositionType CUserMapsLayer::pointPositionToLine(const QPointF &clickedPosition, int &index1, int &index2)
{
    /* // added for testing purposes
    QPointF pontTest1 = QPointF(50.0, 250.0);
    QPointF pontTest2 = QPointF(50.0, 350.0);
    QPointF pontTest3 = QPointF(200.0, 220.0);
    QVector<QPointF> linePoints;
    linePoints.insert(0, pontTest1);
    linePoints.insert(1, pontTest2);
    linePoints.insert(2, pontTest3);
    m_selectedObjPoints = linePoints;*/

    QPointF pointA;
    QPointF pointB;
    qreal linePoint;
    int objPointsIndex;

    for ( objPointsIndex = 0; objPointsIndex < m_selectedObjPoints.size() - 1; objPointsIndex++ )
    {
        pointA = m_selectedObjPoints[objPointsIndex];
        pointB = m_selectedObjPoints[objPointsIndex+1];

        if ( qAbs(clickedPosition.x() - pointA.x()) <= PIXEL_OFFSET &&
             qAbs(clickedPosition.y() - pointA.y()) <= PIXEL_OFFSET)
        {
             // clicked close to pointA
             // save indices of pointA and pointB in index1 and index2
             index1 = objPointsIndex;
             index2 = objPointsIndex + 1;
             return EPointPositionType::AtSpecificPoint;
        }
        else if ( qAbs(clickedPosition.x() - pointB.x()) <= PIXEL_OFFSET &&
                  qAbs(clickedPosition.y() - pointB.y()) <= PIXEL_OFFSET)
        {
            // clicked close to pointB
            // saves indices of pointB and pointA in index1 and index2
            // (whereas the first saved point is the point where clicked)
            index1 = objPointsIndex + 1;
            index2 = objPointsIndex;
            return EPointPositionType::AtSpecificPoint;
        }

        if (pointA.x() <= pointB.x() + PIXEL_OFFSET)
        {
            // check whether selected point lies between pointA and pointB
            if ((clickedPosition.x() + PIXEL_OFFSET >= pointA.x()) && (clickedPosition.x() <= pointB.x() + PIXEL_OFFSET) )
            {
                if ( (clickedPosition.y() + PIXEL_OFFSET >= pointA.y() && clickedPosition.y() <= pointB.y()) + PIXEL_OFFSET )
                {
                    // clicked position lies between pointA and pointB
                    // clicked position lies between pointA and pointB
                    // check if clicked position lies on line
                    linePoint = calculateYaxisValueOnLine(pointA, pointB, clickedPosition);

                    // check if calculated point lies around selected line
                    if (qAbs(clickedPosition.y() - linePoint) <= PIXEL_OFFSET)
                    {
                        index1 = -1;
                        index2 = -1;
                        return EPointPositionType::OnLine;                        
                    }
                    else
                    {
                        index1 = -1;
                        index2 = -1;
                        return EPointPositionType::NotOnLine;                        
                    }
                }
            }
        }
        else if (pointA.x() + PIXEL_OFFSET > pointB.x())
        {
                // check whether selected point lies between pointA and pointB
                if ((clickedPosition.x() + PIXEL_OFFSET >= pointB.x()) && (clickedPosition.x() <= pointA.x() + PIXEL_OFFSET) )
                {
                    if ( (clickedPosition.y() <= pointB.y() + PIXEL_OFFSET && clickedPosition.y() + PIXEL_OFFSET >= pointA.y()) )
                    {
                        // clicked position lies between pointA and pointB
                        // check if clicked position lies on line
                        linePoint = calculateYaxisValueOnLine(pointA, pointB, clickedPosition);

                        // check if calculated point lies around selected line
                        if (qAbs(clickedPosition.y() - linePoint) <= PIXEL_OFFSET)
                        {
                            index1 = -1;
                            index2 = -1;
                            return EPointPositionType::OnLine;
                        }
                        else
                        {
                            index1 = -1;
                            index2 = -1;
                            return EPointPositionType::NotOnLine;
                        }
                    }
                }
         }
    }
    index1 = -1;
    index2 = -1;
    return EPointPositionType::NotOnLine;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         qreal CUserMapsLayer::calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint)
///
/// \brief      Calculates yAxis point value for given XAxis point of linear function.
///
/// \param		pointA - First point used to determine linear function equation.
/// \param      pointB - Second point used to determine linear function equation.
/// \param      clickedPoint - Clicked point.
///
/// \return		qreal Value on YAxis calculated using linear function equation.
qreal CUserMapsLayer::calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint)
{
    // line equation between two points
    // m = (yA − yB)/(xA − xB)
    // y - yA = m (x − xA)
    // y = mx - mxA + yA
    // b= -mxA + yA
    qreal m, b;
    if (qAbs(pointA.x() - pointB.x()) > 0)
    {
        // not vertical line
        m = (pointA.y() - pointB.y())/(pointA.x() - pointB.x());
    }
    else
    {
        // vertical line
        return clickedPoint.y();
    }
    b = -m * pointA.x() + pointA.y();
    return m * clickedPoint.x() + b;
}


