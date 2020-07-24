////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsrenderer.h
///
///	\author Elreg
///
///	\brief	Definition of the CUserMapsLayer class which represents
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2019.
////////////////////////////////////////////////////////////////////////////////

#include "usermapslayer.h"
#include "usermapsrenderer.h"
#include <QDebug>
#include <QSharedPointer>
#include "../LayerLib/coordinates.h"
#include <QDateTime>
#include "usermapsmanager.h"

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
        if (! CUserMapsManager::getEditModeOnStat())
		return;

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
/// \param		event Mouse event.
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
                convertGeoPointToPixelVector(CUserMapsManager::getObjPositionStat(), m_selectedObjPoints);
		break;
	case EUserMapObjectType::Area:
	case EUserMapObjectType::Line:
                convertGeoVectorToPixelVector(CUserMapsManager::getObjPointsVectorStat(), m_selectedObjPoints);
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
///
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
/// \brief      Converts vectro of pixel coordinates into vector of geo coordinates.
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

        if (CUserMapsManager::getObjSelectedStat())
	{
		//TODO: probably deselect on click?
	}
	else
	{
                CUserMapsManager::selectObjectStat(pos);
	}
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
