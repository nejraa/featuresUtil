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
#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include "../OpenGLBaseLib/imagetexture.h"
#include "../OpenGLBaseLib/vertexbuffer.h"
#include "../OpenGLBaseLib/imagetexture.h"
#include "../OpenGLBaseLib/vertexbuffer.h"

class CUserMapsRenderer : public CBaseRenderer, public QObject
{
public:
	CUserMapsRenderer();
	~CUserMapsRenderer();

	void render() override;
	void synchronize( QQuickFramebufferObject* item ) override;
	void initializeGL() override;
    virtual void renderPrimitives( QOpenGLFunctions* func ) override;
    virtual void renderTextures() override;
    // Updates
    void updatePoint();
    void updateLine();
    void updateCircle();
    void updatePolygon();
    // Draws
    void drawPoint( QOpenGLFunctions* func );
    void drawLine( QOpenGLFunctions* func );
    void drawCircle( QOpenGLFunctions* func );
    void drawPolygon( QOpenGLFunctions* func );




private:
    QVector4D m_PointColour;					///< Point colour
    QVector4D m_LineColour;					    ///<Line colour
    QVector4D m_CircleColour;					///<Circle colour
    QVector4D m_PolygonColour;					///<Polygon colour
    QSharedPointer<CVertexBuffer> m_PointBuf;	///< OpenGL vertex buffer (vertices and colour) to draw points
    CImageTexture* m_pTexture;  ///< image used as a textures

    // Lines buffer
    QSharedPointer<CVertexBuffer> m_LineBuf;	///< VBO used to draw Lines

    //circle buffer
    QSharedPointer<CVertexBuffer> m_CircleBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw circles

    //polygon buffer
    QSharedPointer<CVertexBuffer> m_PolygonBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw polygons

    QOpenGLDebugLogger *m_pOpenGLLogger;	///< OpenGL error logger

    void logOpenGLErrors();
};

#endif // CUSERMAPSRENDERER_H
