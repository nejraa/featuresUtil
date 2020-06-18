////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsrenderer.h
///
///	\author Elreg
///
///	\brief	Declaration of the CUserMapsRenderer class which renders
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2019.
////////////////////////////////////////////////////////////////////////////////

#ifndef CUSERMAPSRENDERER_H
#define CUSERMAPSRENDERER_H

#include "baserenderer.h"

class CUserMapsRenderer : public CBaseRenderer
{
public:
	CUserMapsRenderer();
	~CUserMapsRenderer();

	void render() override;
	void synchronize( QQuickFramebufferObject* item ) override;
	void initializeGL() override;
	void renderPrimitives( QOpenGLFunctions* func ) override;
	void renderTextures() override;
};

#endif // CUSERMAPSRENDERER_H
