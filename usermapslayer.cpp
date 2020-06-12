#include "usermapslayer.h"
#include "usermapsrenderer.h"

CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
	: CBaseLayer(parent)
{

}

CUserMapsLayer::~CUserMapsLayer()
{

}

void CUserMapsLayer::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
}

void CUserMapsLayer::mouseMoveEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
}

void CUserMapsLayer::mouseReleaseEvent(QMouseEvent *event)
{
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
