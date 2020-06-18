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
#include "../NavUtilsLib/coordinates.h"
#include <QDateTime>
#include "usermapeditor.h"
#include "usermapsmanager.h"
#include "usermappointeditor.h"

const int MOVE_EVT_TIME_LIMIT = 500;
const int LONG_PRESS_DURATION_MS = 2000;

CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
	: CBaseLayer(parent)
	, m_moveEvtTimestamp(0)
{
	setAcceptedMouseButtons(Qt::AllButtons);
	QObject::connect(CUserMapsManager::instance(), &CUserMapsManager::selectedObjTypeChanged,
					 this, &CUserMapsLayer::setSelectedObjType);
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
	m_onPressTimer.start(LONG_PRESS_DURATION_MS);
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
	if(m_onPressTimer.isActive() && !m_isCursorMoving)
		onPositionClicked(event->screenPos());

	updateObjectPosition();
	m_isCursorMoving = false;
}

void CUserMapsLayer::pressTimerTimeout()
{
	if(!m_isCursorMoving)
		qDebug() << "Long press";
	m_onPressTimer.stop();
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
void CUserMapsLayer::setSelectedObjType(EUserMapObjectType objType)
{
	m_objectType = objType;
	switch (objType)
	{
	case EUserMapObjectType::Point:
	case EUserMapObjectType::Circle:
		convertGeoPointToPixelVector(CUserMapsManager::getObjPosition(), m_selectedObjPoints);
		break;
	case EUserMapObjectType::Area:
	case EUserMapObjectType::Line:
		convertGeoVectorToPixelVector(CUserMapsManager::getObjPointsVector(), m_selectedObjPoints);
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
	CCoordinates c;
	pixelPoints.clear();
	pixelPoints.reserve(geoPoints.size());
	for (const CPosition &point : geoPoints)
		pixelPoints.append(c.convertGeoCoordsToPixelCoords(point));
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
	CCoordinates c;
	pixelPoints.clear();
	pixelPoints.append(c.convertGeoCoordsToPixelCoords(geoPoint));
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
	CCoordinates c;
	geoPoints.clear();
	geoPoints.reserve(pixelVector.size());
	for (const QPointF &point : pixelVector)
		geoPoints.append(c.convertPixelCoordsToGeoCoords(point));
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
	CCoordinates c;
	CUserMapsManager::selectObject(c.convertPixelCoordsToGeoCoords(clickedPosition));
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
		CUserMapsManager::setObjPosition(geoPoints[0]);
		break;
	case EUserMapObjectType::Area:
	case EUserMapObjectType::Line:
		CUserMapsManager::setObjPointsVector(geoPoints);
		break;
	}
}
