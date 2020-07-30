////////////////////////////////////////////////////////////////////////////////
/// \file   usermapslayer.cpp
///
/// \author Elreg
///
/// \brief  Implementation of the CUserMapsLayer class which represents
///         the user maps layer.
///
/// (C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#include "usermapslayer.h"
#include "usermapsrenderer.h"
#include <QDebug>
#include <QtMath>
#include <QSharedPointer>
#include "../LayerLib/coordinates.h"
#include <QDateTime>
#include "usermapsmanager.h"
#include <QColor>
#include <QDebug>

#include "../LayerLib/corelayer.h"
#include "../LayerLib/viewcoordinates.h"

const int PIXEL_OFFSET				= 10;	///< Pixel offset (tolerance) for mouse click event. Click can be this far from a certain position of interest, and still be accepted.
const int MOVE_EVT_TIME_LIMIT		= 20;	///< Mouse move event will not be accepted and processed more often than defined my this interval in miliseconds.
const int MOVE_EVT_PIXEL_THRESHOLD	= 20;	///< Threshold distance in pixels for mouse move event to be processed as a move event.
const int LONG_PRESS_DURATION_MS	= 1000; ///< Time threshold for press and hold to be processed as a long press action.

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
///
/// \brief  Constructor.
///
/// \param  parent - parent QQuickItem.
////////////////////////////////////////////////////////////////////////////////
CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
    : CBaseLayer(parent)
    , m_isCursorMoving(false)
    , m_isLongMousePress(false)
    , m_objectType(EUserMapObjectType::Unkown_Object)
    , m_moveEvtTimestamp(0)
    , m_pointPositionType(EPointPositionType::Unknown)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    QObject::connect(CUserMapsManager::instance(), &CUserMapsManager::selectedObjChanged,
                     this, &CUserMapsLayer::setSelectedObject);

    m_onPressTimer.setSingleShot(true);
    m_onPressTimer.setInterval(LONG_PRESS_DURATION_MS);
    connect(&m_onPressTimer, &QTimer::timeout, this, &CUserMapsLayer::pressTimerTimeout);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsLayer::~CUserMapsLayer()
///
/// \brief  Destructor.
////////////////////////////////////////////////////////////////////////////////
CUserMapsLayer::~CUserMapsLayer()
{

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     void COwnshipLayer::initialise()
///
/// \brief  Initialises signals and slots.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::initialise()
{
	if ( !m_initialised )
	{
		// Find the CoreLayer object. This will be a child of my parent (sibling)
		CCoreLayer* pCoreLayer = parent()->parent()->findChild<CCoreLayer*>();

		// Connect to the CCoreLayer range changed signal
		if ( pCoreLayer )
		{
			if ( connect( pCoreLayer, &CCoreLayer::offsetChanged, this, &CUserMapsLayer::onOffsetChanged ) )
			{
				qDebug() << "CUserMapLayer: Failed to connect to CCoreLayer offsetChanged signal";
			}
		}

		m_initialised = true;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void    CUserMapsLayer::onOffsetChanged()
///
/// \brief      Handles a change in offset from CCoreLayer.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::onOffsetChanged()
{
	update();
}

/// \fn void CUserMapsLayer::mousePressEvent(QMouseEvent *event)
///
/// \brief  Mouse press event handler. Does nothing if app is not in map editing mode.
///
/// \param  event - Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mousePressEvent(QMouseEvent *event)
{
    if (! CUserMapsManager::getEditModeOnStat())
        return;

    m_onPressTimer.start();
    m_moveEvtStartPoint = event->screenPos();
    m_isCursorMoving = false;
    m_isLongMousePress = false;

    // sets member variables for clicked point position estimation
    m_pointPositionType = checkPointPositionToObj(m_moveEvtStartPoint, m_index1, m_index2);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
///
///
/// \brief  Mouse move event handler. Does nothing if app is not in map editing mode.
///
/// \param  event - Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
{
    if (! CUserMapsManager::getEditModeOnStat())
        return;

    // Checking whether cursor has moved enough since press event for this to be considered an intentional move event.
    const QPointF currentPos = event->screenPos();
    const bool cursorMoved = qAbs(m_moveEvtStartPoint.x() - currentPos.x()) > MOVE_EVT_PIXEL_THRESHOLD ||
            qAbs(m_moveEvtStartPoint.y() - currentPos.y()) > MOVE_EVT_PIXEL_THRESHOLD;

    // Checking whether enough time has passed since processing the move event last time.
    const qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    const bool updateAllowed = currentTimestamp - m_moveEvtTimestamp > MOVE_EVT_TIME_LIMIT;

    bool processMoveEvent = (cursorMoved || m_isCursorMoving) && updateAllowed && ! m_isLongMousePress;
    if (! processMoveEvent)
        return;

    m_onPressTimer.stop();
    m_isCursorMoving = true;
    m_isLongMousePress = false;
    m_moveEvtTimestamp = currentTimestamp;

    handleObjAction(m_moveEvtStartPoint, currentPos);
    m_moveEvtStartPoint = currentPos;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
///
///
/// \brief  Mouse release event handler. Does nothing if app is not in map editing mode.
///
/// \param  event - Mouse event.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
{
    if (! CUserMapsManager::getEditModeOnStat())
        return;

    if(! m_isLongMousePress && ! m_isCursorMoving)
        onPositionClicked(event->screenPos());

    if (m_isCursorMoving)
        updateObjectPosition();

    m_isCursorMoving = false;
    m_isLongMousePress = false;
    m_onPressTimer.stop();
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::pressTimerTimeout()
///
/// \brief  Long press timer timeout handler. If move event has not been registered,
///         this function sets m_isLongMousePress flag.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::pressTimerTimeout()
{
    if (! m_isCursorMoving)
        m_isLongMousePress = true;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn QQuickFramebufferObject::Renderer* CUserMapsLayer::createRenderer() const
///
/// \brief  Creates the renderer that's associated with user maps layer.
///
/// \return Pointer to the renderer.
////////////////////////////////////////////////////////////////////////////////
QQuickFramebufferObject::Renderer *CUserMapsLayer::createRenderer() const
{
    return new CUserMapsRenderer();
}

////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsLayer::handleObjAction(const QPointF &initialPosition, const QPointF &endPosition)
///
/// \brief  Handling press/move events
///         - if long mouse press at specific point (for line/area object) - deleting point,
///         - if mouse press and move at specific point (for line/area object) - moving point,
///         - if long press on line (for line/area object) - adding line point,
///         - if mouse press and move on line (for line/area object) - moving line points,
///         - if mouse press and move on line (for circle object) - resize circle (change radius),
///         - if mouse press and move inside object - moving object,
///         - if mouse press outside object - deselect object.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::handleObjAction(const QPointF &initialPosition, const QPointF &endPosition)
{
    switch (m_objectType)
    {
    case EUserMapObjectType::Area:
        handleAreaObjAction(initialPosition, endPosition);
        break;

    case EUserMapObjectType::Line:
        handleLineObjAction(initialPosition, endPosition);
        break;

    case EUserMapObjectType::Circle:
        handleCircleObjAction(initialPosition, endPosition);
        break;

    case EUserMapObjectType::Point:
        handlePointObjAction(initialPosition, endPosition);
        break;

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::handleAreaObjAction(const QPointF &initialPosition, const QPointF &endPosition)
///
/// \brief  Performs action on area object. Required action is derived by looking at different
///         member variables (re)set during press, move, release and long press mouse events.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::handleAreaObjAction(const QPointF &initialPosition, const QPointF &endPosition)
{
    switch (m_pointPositionType)
    {
    case EPointPositionType::AtSpecificPoint:
    {
        if (m_isLongMousePress)
            deleteObjPoint(m_index1);
        else if (m_isCursorMoving)
            moveObjPoint(initialPosition, endPosition, m_index1);
        break;
    }

    case EPointPositionType::OnLine:
    {
        if (m_isLongMousePress)
            insertObjPoint(m_index1 + 1, initialPosition);
        else if (m_isCursorMoving)
            moveObjPoints(initialPosition, endPosition, m_index1, m_index2);
        break;
    }

    case EPointPositionType::InsideObject:
    {
        if (m_isCursorMoving)
            moveObj(initialPosition, endPosition);
        break;
    }

    case EPointPositionType::OutsideObject:
    {
        if (CUserMapsManager::getObjSelectedStat())
            CUserMapsManager::deselectObjectStat();
        break;
    }

    default:
        // no action
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::handleLineObjAction(const QPointF &initialPosition, const QPointF &endPosition)
///
/// \brief  Performs action on line object. Required action is derived by looking at different
///         member variables (re)set during press, move, release and long press mouse events.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::handleLineObjAction(const QPointF &initialPosition, const QPointF &endPosition)
{
    switch (m_pointPositionType)
    {
    case EPointPositionType::AtSpecificPoint:
    {
        if (m_isLongMousePress)
            deleteObjPoint(m_index1);
        else if (m_isCursorMoving)
            moveObjPoint(initialPosition, endPosition, m_index1);
        break;
    }

    case EPointPositionType::OnLine:
    {
        if (m_isLongMousePress)
            insertObjPoint(m_index1 + 1, initialPosition);
        else if (m_isCursorMoving)
            moveObj(initialPosition, endPosition);
        break;
    }

    case EPointPositionType::NotOnLine:
        if (CUserMapsManager::getObjSelectedStat())
            CUserMapsManager::deselectObjectStat();
        break;

    default:
        // no action
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::handleCircleObjAction(const QPointF &initialPosition, const QPointF &endPosition)
///
/// \brief  Performs action on circle object. Required action is derived by looking at different
///         member variables (re)set during press, move, release and long press mouse events.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::handleCircleObjAction(const QPointF &initialPosition, const QPointF &endPosition)
{
    switch (m_pointPositionType)
    {
    case EPointPositionType::InsideObject:
    {
        if (m_isCursorMoving)
            moveObj(initialPosition, endPosition);
        break;
    }

    case EPointPositionType::OnLine:
    {
        // Resize object
        if (m_isCursorMoving)
        {
            // Calculates the distance between endPosition and center point and uses that as new radius value.
            CPosition centerPoint = CUserMapsManager::getObjPositionStat();
            QPointF circleCenterPixel = convertGeoPointToPixelPoint(centerPoint);

            float updatedRadius = qSqrt( qPow(circleCenterPixel.x() - endPosition.x(), 2) + qPow(circleCenterPixel.y() - endPosition.y(), 2) );

            // convert pixels to NM
            updatedRadius = updatedRadius * CViewCoordinates::getPixelsToNauticalMiles();
            CUserMapsManager::setObjRadiusStat(updatedRadius);
        }
        break;
    }

    case EPointPositionType::OutsideObject:
        if (CUserMapsManager::getObjSelectedStat())
            CUserMapsManager::deselectObjectStat();
        break;

    default:
        // no action
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::handlePointObjAction(const QPointF &initialPosition, const QPointF &endPosition)
///
/// \brief  Moves point object if cursor is moving.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::handlePointObjAction(const QPointF &initialPosition, const QPointF &endPosition)
{
    if (!m_isCursorMoving)
        return;

    moveObj(initialPosition, endPosition);
    return;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::moveObj(const QPointF &initialPosition, const QPointF &endPosition)
///
/// \brief  Moves all points of the selected object by the difference of endPosition
///         and initialPosition.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::moveObj(const QPointF &initialPosition, const QPointF &endPosition)
{	
    if ( m_selectedObjPoints.isEmpty())
        return;

    QPointF pointDifference = endPosition - initialPosition;
    for (int i = 0; i < m_selectedObjPoints.size(); i++)
        m_selectedObjPoints[i] += pointDifference;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::moveObjPoint(const QPointF &initialPosition, const QPointF &endPosition, const int index)
///
/// \brief  Moves single point of the selected object which is located at provided index.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
///         index - Index of selected point.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::moveObjPoint(const QPointF &initialPosition, const QPointF &endPosition, const int index)
{
    if ( m_selectedObjPoints.isEmpty() )
        return;

    if (index > m_selectedObjPoints.size() || index < 0 )
        return;

    QPointF pointDifference = endPosition - initialPosition;
    m_selectedObjPoints[index] += pointDifference;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::moveObjPoints(const QPointF &initialPosition,
///                         const QPointF &endPosition, const int index1, const int index2)
///
/// \brief  Moves two points forming line segment if mouse press occurred on line or area object.
///
/// \param  initialPosition - Start click position.
///         endPosition - End click position.
///         index1 - Index of the first point.
///         index2 - Index of the second point.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::moveObjPoints(const QPointF &initialPosition, const QPointF &endPosition,
                                   const int index1, const int index2)
{
    if ( m_selectedObjPoints.isEmpty() )
        return;

    // Check if consecutive points
    if ( ( qAbs(index1 - index2) != 1 ) || index1 > m_selectedObjPoints.size()
         || index2 > m_selectedObjPoints.size()
         || index1 < 0 || index2 < 0)
        return;

    QPointF pointDifference = endPosition - initialPosition;
    m_selectedObjPoints[index1] += pointDifference;
    m_selectedObjPoints[index2] += pointDifference;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::deleteObjPoint(const int index)
///
/// \brief  Deletes point from given index position.
///
/// \param  index - Vector index.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::deleteObjPoint(const int index)
{    
    if ( m_selectedObjPoints.isEmpty() )
        return;

    if ( (index < 0) || ( index > m_selectedObjPoints.size() ) )
        return;

    // Not allowing point removal if line object is composed of only less than 3 points.
    if ( m_objectType == EUserMapObjectType::Line && m_selectedObjPoints.size() < 3)
        return;

    // Not allowing point removal if area object is composed of only less than 4 points.
    if ( m_objectType == EUserMapObjectType::Area && m_selectedObjPoints.size() < 4)
        return;

    m_selectedObjPoints.removeAt(index);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::insertObjPoint(const int index, const QPointF &pos)
///
/// \brief  Inserts point at index position.
///
/// \param  index - vector index.
///			pos - Position of point to be added.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::insertObjPoint(const int index, const QPointF &pos)
{
    if (index > 0 && index < m_selectedObjPoints.size() + 1 )
        m_selectedObjPoints.insert(index, pos);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn EPointPositionType CUserMapsLayer::checkPointPosition(const QPointF &clickedPosition,
///                                       int &index1, int &index2)
///
/// \brief  Checks whether clicked point lies inside selected object boundaries (Line and Area object).
///
/// \param  clickedPoint - Clicked point.
///         index1 - Index of the first point on line segment of area/line object where clicked position lies.
///         index2 - Index of the second point on line segment of area/line object where clicked position lies.
///
/// \return Position of clicked point with respect to an object.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::checkPointPositionToObj(const QPointF &clickedPosition,
                                                           int &index1, int &index2)
{
    index1 = -1;
    index2 = -1;
    switch (m_objectType)
    {
    case EUserMapObjectType::Area:
        return pointPositionToArea(clickedPosition, index1, index2);

    case EUserMapObjectType::Line:
        return pointPositionToLine(clickedPosition, index1, index2);

    case EUserMapObjectType::Circle:
        return pointPositionToCircle(clickedPosition);

    case EUserMapObjectType::Point:
        return pointPositionToPointObj(clickedPosition);

    default:
        return EPointPositionType::Unknown;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::selectedObjType(bool isObjSelected, EUserMapObjectType objType)
///
/// \brief  Based on object type it gets the position points.
///
/// \param  isObjSelected - Flag indicating whether an object is selected or not.
///         objType - Type of object.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::setSelectedObject(bool isObjSelected, EUserMapObjectType objType)
{
    if (! isObjSelected)
        return;

    m_objectType = objType;
    switch (objType)
    {
    case EUserMapObjectType::Point:
    case EUserMapObjectType::Circle:
        convertGeoPointToPixelVector(CUserMapsManager::getObjPositionStat(), m_selectedObjPoints);
        break;

    case EUserMapObjectType::Area:
    case EUserMapObjectType::Line:
        convertGeoVectorToPixelVector(CUserMapsManager::getObjPointsVectorStat(), m_selectedObjPoints);
        break;

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoint,
///														  QVector<QPointF> &pixelPoints)
///
/// \brief  Converts vector of Geo coordinates into vector of pixel coordinates.
///
/// \param  geoPoints - Geo coordinates.
///         pixelPoints - Pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints,
                                                   QVector<QPointF> &pixelPoints)
{
    pixelPoints.clear();
    pixelPoints.reserve(geoPoints.size());
    for (const CPosition &geoPoint : geoPoints)
        pixelPoints.append(convertGeoPointToPixelPoint(geoPoint));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::convertGeoPointToPixelVector(const CPosition &geoPoint,
///														     QVector<QPointF> &pixelPoints)
/// \brief  Converts Geo point into pixel vector.
///
/// \param  geoPoint - Geo coordinates.
///         pixelPoints - Pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoPointToPixelVector(const CPosition &geoPoint,
                                                  QVector<QPointF> &pixelPoints)
{
    pixelPoints.clear();
    pixelPoints.append(convertGeoPointToPixelPoint(geoPoint));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector,
///                                                 QVector<CPosition> &geoPoints)
/// \brief  Converts vector of pixel coordinates into vector of geo coordinates.
///
/// \param  pixelVector - Vector of pixel coordinates.
///
/// \return Vector with geo coordinates.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector,
                                                   QVector<CPosition> &geoPoints)
{
    geoPoints.clear();
    geoPoints.reserve(pixelVector.size());
    for (const QPointF &pixelPoint : pixelVector)
        geoPoints.append(convertPixelPointToGeoPoint(pixelPoint));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn CPosition CUserMapsLayer::convertPixelPointToGeoPoint(const QPointF &pixelPoint)
///
/// \brief  Converts pixel point into geo point.
///
/// \param  pixelPoint - Pixel coordinate to be converted to geo coordinate.
///
/// \return Geo coordinate.
////////////////////////////////////////////////////////////////////////////////
CPosition CUserMapsLayer::convertPixelPointToGeoPoint(const QPointF &pixelPoint)
{
    GEOGRAPHICAL lat;
    GEOGRAPHICAL lon;
    CViewCoordinates::Instance()->Convert(PIXEL(pixelPoint.x()), PIXEL(pixelPoint.y()), lat, lon);
    return CPosition(double(lat), double(lon));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn QPointF CUserMapsLayer::convertGeoPointToPixelPoint(const CPosition &geoPoint)
///
/// \brief  Converts geo coordinate into pixel coordinate.
///
/// \param  geoPoint - Point in geo coordinates.
///
/// \return Point in pixel coordinates.
////////////////////////////////////////////////////////////////////////////////
QPointF CUserMapsLayer::convertGeoPointToPixelPoint(const CPosition &geoPoint)
{
    PIXEL x = 0.0;
    PIXEL y = 0.0;
    CViewCoordinates::Instance()->Convert(GEOGRAPHICAL(geoPoint.Latitude()), GEOGRAPHICAL(geoPoint.Longitude()), x, y);
    return QPointF(x,y);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::onPositionClicked(const QPointF &clickedPosition)
///
/// \brief  When user clicks once.
///
/// \param  clickedPosition Position where user has clicked.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::onPositionClicked(const QPointF &clickedPosition)
{
    CPosition pos = convertPixelPointToGeoPoint(clickedPosition);

    if (CUserMapsManager::getCreatingNewObjStat())
    {
        CUserMapsManager::addObjPointStat(pos);
        return;
    }

    if (CUserMapsManager::getObjSelectedStat())
    {
        // deselect object
        CUserMapsManager::deselectObjectStat();
    }
    else
    {
        // CUserMapsManager::selectObjectStat(); // TODO: will be done in CUserMapsRenderer instead (using OpenGL).
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsLayer::updateObjectPosition()
///
/// \brief  Updates object position based on its type.
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

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn EPointPositionType CUserMapsLayer::pointPositionToArea(const QPointF &clickedPosition,
///                                         int &index1, int &index2)
///
/// \brief  Returns position of clicked point with respect to an the selected object.
///
/// \param  clickedPosition - Clicked position in pixel coordinates.
///         index1 - Index of the first point on area object between segments where clicked position lies.
///         index2 - Index of the second point on area object between segments where clicked position lies.
///
/// \return Selected point position with respect to the selected area.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::pointPositionToArea(const QPointF &clickedPosition,
                                                       int &index1, int &index2)
{
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

        qreal yPointCalc = calculateYaxisValueOnLine(pointC, pointD, clickedPosition);
        if ( qAbs(clickedPosition.y() - yPointCalc ) < PIXEL_OFFSET  &&
             ( (clickedPosition.x() < PIXEL_OFFSET + pointC.x() && clickedPosition.x() + PIXEL_OFFSET > pointD.x()) ||
               (clickedPosition.x() + PIXEL_OFFSET > pointC.x() && clickedPosition.x() < pointD.x() + PIXEL_OFFSET ) ) )
        {
            index1 = i;
            index2 = i+1;
            return EPointPositionType::OnLine;
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
/// \fn EPointPositionType CUserMapsLayer::pointPositionToCircle(const QPointF &clickedPosition)
///
/// \brief  Returns position of clicked point with respect to the selected circle object.
///
/// \param  clickedPosition - Clicked position in pixel coordinates.
///
/// \return Selected point position with respect to the selected circle.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::pointPositionToCircle(const QPointF &clickedPosition)
{
    float radiusVal = CUserMapsManager::getObjRadiusStat();
    // radiusVal - radius of circle in nautical miles
    // conversion to pixel coordinates
    radiusVal = radiusVal * CViewCoordinates::getNauticalMilesToPixels();
    CPosition centerPoint = CUserMapsManager::getObjPositionStat();
    QPointF circleCenterPixel = convertGeoPointToPixelPoint(centerPoint);

    qreal distance = qSqrt(qPow(clickedPosition.x() - circleCenterPixel.x(), 2) + qPow(clickedPosition.y() - circleCenterPixel.y(), 2));
    if (qAbs(distance - radiusVal) <= PIXEL_OFFSET)
        return EPointPositionType::OnLine;
    else if (distance > radiusVal)
        return  EPointPositionType::OutsideObject;
    else if ( distance < radiusVal )
        return EPointPositionType::InsideObject;
    else
        return EPointPositionType::Unknown;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn EPointPositionType CUserMapsLayer::pointPositionToLine(const QPointF &clickedPosition,
///                                                 int &index1, int &index2)
///
/// \brief  Returns position of clicked point with respect to the selected line object.
///
/// \param  clickedPosition - Clicked position in pixel coordinates.
///         index1 - Index of the first point on line object on segment where clicked position lies.
///         index2 - Index of the second point on line object on segment where clicked position lies.
///
/// \return Selected point position with respect to the selected line.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::pointPositionToLine(const QPointF &clickedPosition,
                                                       int &index1, int &index2)
{
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
                        index1 = objPointsIndex;
                        index2 = objPointsIndex + 1;
                        return EPointPositionType::OnLine;
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
                        index1 = objPointsIndex;
                        index2 = objPointsIndex + 1;
                        return EPointPositionType::OnLine;
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
/// \fn EPointPositionType CUserMapsLayer::pointPositionToPointObj(const QPointF &clickedPosition)
///
/// \brief  Returns position of clicked point with respect to the selected point object.
///
/// \param  clickedPosition - Clicked position in pixel coordinates.
///
/// \return Selected point position with respect to the selected point object.
////////////////////////////////////////////////////////////////////////////////
EPointPositionType CUserMapsLayer::pointPositionToPointObj(const QPointF &clickedPosition)
{
    // checking whether clicked point lies around fixed pixel offset around the point or not
    // considering offset dependant on icon size can be considered as well
    if ( qAbs(clickedPosition.x() - m_selectedObjPoints[0].x()) < PIXEL_OFFSET &&
         qAbs(clickedPosition.y() - m_selectedObjPoints[0].y()) < PIXEL_OFFSET )
    {
        return EPointPositionType::InsideObject;
    }
    else
        return EPointPositionType::OutsideObject;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn qreal CUserMapsLayer::calculateYaxisValueOnLine(const QPointF &pointA,
///                                 const QPointF &pointB, const QPointF clickedPoint)
///
/// \brief  Calculates yAxis point value for given XAxis point of linear function.
///
/// \param  pointA - First point used to determine linear function equation.
///         pointB - Second point used to determine linear function equation.
///         clickedPoint - Clicked point.
///
/// \return Value on YAxis calculated using linear function equation.
////////////////////////////////////////////////////////////////////////////////
qreal CUserMapsLayer::calculateYaxisValueOnLine(const QPointF &pointA,
                                      const QPointF &pointB, const QPointF clickedPoint)
{
    // line equation between two points
    // m = (yA - yB)/(xA - xB)
    // y - yA = m (x - xA)
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
        qDebug() << "Vertical line: " << clickedPoint.y();
    }
    b = -m * pointA.x() + pointA.y();
    return m * clickedPoint.x() + b;
}
