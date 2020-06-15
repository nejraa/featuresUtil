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
}

CUserMapsLayer::~CUserMapsLayer()
{

}

void CUserMapsLayer::mousePressEvent(QMouseEvent *event)
{
	qDebug() << "Entered in mousePressEvent";
	m_onPressTimer.start(2000);
	Q_UNUSED(event);
}

void CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
{
	if(m_onPressTimer.isActive() || m_isMoving)
	{
		m_onPressTimer.stop();
		m_isMoving = true;

		CCoordinates c;
		CPosition pixelPosition = c.convertPixelCoordsToGeoCoords(event->screenPos());

		QSharedPointer<CUserMapEditor> pEditor = CUserMapsManager::getMapEditor().toStrongRef();

		if(pEditor)
		{
			EUserMapObjectType objectType = pEditor->getSelectedObjectType();
			switch (objectType)
			{
			case(EUserMapObjectType::Point):
			case(EUserMapObjectType::Circle):
				pEditor->setObjPosition(pixelPosition);
				break;
			case(EUserMapObjectType::Line):
			case(EUserMapObjectType::Area):
				//Function that accepts vector of points to set position
				break;
			}
		}
	}

	Q_UNUSED(event);
}

void CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
{
	m_isMoving = false;
	qDebug() << "Entered in mouseReleaseEvent";
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
