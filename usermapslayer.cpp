#include "usermapslayer.h"
#include "usermapsrenderer.h"
#include <QDebug>
#include <QSharedPointer>
#include "../NavUtilsLib/coordinates.h"

#include "usermapeditor.h"
#include "usermapsmanager.h"
#include "usermappointeditor.h"

CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
	: CBaseLayer(parent)
{
	setAcceptedMouseButtons(Qt::AllButtons);
	QObject::connect(CUserMapsManager::instance(), &CUserMapsManager::selectedObjTypeChanged,
					 this, &CUserMapsLayer::selectedObjType);
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
	m_onPressTimer.start(2000);
	m_startPoint = event->screenPos();
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
	if(m_onPressTimer.isActive() || m_isMoving)
	{
		m_onPressTimer.stop();
		m_isMoving = true;

		QPointF pointDifference = m_startPoint - event->screenPos();
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
	setObjectPosition();
	m_isMoving = false; //Do i need this?
	Q_UNUSED(event);
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
void CUserMapsLayer::selectedObjType(EUserMapObjectType objType)
{
	m_objectType = objType;
	switch (objType)
	{
	case EUserMapObjectType::Point:
	case EUserMapObjectType::Circle:
		convertGeoPointToPixelVector(CUserMapsManager::getObjPosition());
		break;
	case EUserMapObjectType::Area:
	case EUserMapObjectType::Line:
		convertGeoVectorToPixelVector(CUserMapsManager::getObjPointsVector());
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// \fn		CUserMapsLayer::convertGeoVectorToPixelVector(QVector<CPosition> objPoints)
///
/// \brief	Converts vector of Geo coordinates into vector of pixel coordinates.
///
/// \param	objPoints Vectro of points.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoVectorToPixelVector(QVector<CPosition> objPoints)
{
	CCoordinates c;
	m_selectedObjPoints.clear();
	foreach (CPosition point, objPoints) {
		m_selectedObjPoints.append(c.convertGeoCoordsToPixelCoords(point));
	}
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::convertGeoPointToPixelVector(CPosition objPoint)
///
/// \brief      Converts Geo point into pixel vector.
///
/// \param		objPoint Point of the selected object.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::convertGeoPointToPixelVector(CPosition objPoint)
{
	CCoordinates c;
	m_selectedObjPoints.clear();
	m_selectedObjPoints.append(c.convertGeoCoordsToPixelCoords(objPoint));
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::convertPixelVectorToGeoVector()
///
/// \brief      Converts vectro of pixel coordinates into vector of geo coordinates.
///
/// \return		Vector with geo coordinates.
////////////////////////////////////////////////////////////////////////////////
QVector<CPosition> CUserMapsLayer::convertPixelVectorToGeoVector()
{
	CCoordinates c;
	QVector<CPosition> geoPoints;
	foreach (QPointF point, m_selectedObjPoints)
		geoPoints.append(c.convertPixelCoordsToGeoCoords(point));

	return geoPoints;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn         CUserMapsLayer::setObjectPosition()
///
/// \brief      Sets object position based on his type.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsLayer::setObjectPosition()
{
	QVector<CPosition> objPosition = convertPixelVectorToGeoVector();
	switch (m_objectType)
	{
	case EUserMapObjectType::Point:
	case EUserMapObjectType::Circle:
		CUserMapsManager::setObjPosition(objPosition[0]);
		break;
	case EUserMapObjectType::Area:
	case EUserMapObjectType::Line:
		CUserMapsManager::setObjPointsVector(objPosition);
		break;
	}
}
