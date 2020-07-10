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

#include <QColor>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "../LayerLib/corelayer.h"			// For CCoreLayer
#include "../LayerLib/viewcoordinates.h"

////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
///
/// brief  Constructor
///
////////////////////////////////////////////////////////////////////////////////
CUserMapsLayer::CUserMapsLayer(QQuickItem *parent)
	: CBaseLayer(parent)
{

}

////////////////////////////////////////////////////////////////////////////////
/// fn CUserMapsLayer::~CUserMapsLayer()
///
/// brief Destructor
///
////////////////////////////////////////////////////////////////////////////////
CUserMapsLayer::~CUserMapsLayer()
{

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     COwnshipLayer::initialise()
///
/// \brief  Initialise signals and slots.
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
