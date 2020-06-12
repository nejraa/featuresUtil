#include "usermapsrenderer.h"

CUserMapsRenderer::CUserMapsRenderer()
	: CBaseRenderer("UserMapsView", OGL_TYPE::PROJ_ORTHO)
{

}

CUserMapsRenderer::~CUserMapsRenderer()
{

}

void CUserMapsRenderer::render()
{

}

void CUserMapsRenderer::synchronize(QQuickFramebufferObject *item)
{
    Q_UNUSED(item);
}

void CUserMapsRenderer::initializeGL()
{

}

void CUserMapsRenderer::renderPrimitives(QOpenGLFunctions *func)
{
    Q_UNUSED(func);
}

void CUserMapsRenderer::renderTextures()
{

}
