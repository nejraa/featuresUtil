#include "usermapslayer.h"
#include "usermapsrenderer.h"

CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
	: CBaseLayer(parent)
{

}

CUserMapsLayer::~CUserMapsLayer()
{

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
