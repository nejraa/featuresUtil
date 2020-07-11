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
#include "mapshaderprogram.h"
#include "triangulate.h"
#include <vector>
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
	void updatefillCircle();
	void updatePolygon();
	void updatefillPolygon();
	// Draws
	void drawPoint( QOpenGLFunctions* func );
	void drawLine( QOpenGLFunctions* func );
	void drawCircle( QOpenGLFunctions* func );
	void drawfilledCircle(QOpenGLFunctions* func );
	void drawPolygon( QOpenGLFunctions* func );
	void drawfilledPolygon( QOpenGLFunctions* func );
	void initShader();
	void addText( QString text,double x, double y, QVector4D colour,TextAlignment alignment);

private:
	QVector4D m_PointColour;					///< Point colour
	QVector4D m_LineColour;					    ///<Line colour
	QVector4D m_CircleColour;					///<Circle colour
	QVector4D m_PolygonColour;					///<Polygon colour
	QVector4D m_TextColour;					    ///<Text colour
	QSharedPointer<CVertexBuffer> m_PointBuf;	///< OpenGL vertex buffer (vertices and colour) to draw points
	CImageTexture* m_pTexture;  ///< image used as a textures
	std::vector<GenericVertexData>sortedelements;  //will be deleted
	// Lines buffer
	QSharedPointer<CVertexBuffer> m_LineBuf;	///< VBO used to draw Lines

	//circle buffer
	QSharedPointer<CVertexBuffer> m_CircleBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw circles

	//circle buffer
	QSharedPointer<CVertexBuffer> m_InlineCircleBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw inline

	//polygon buffer
	QSharedPointer<CVertexBuffer> m_PolygonBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw polygons

	//filled polygon buffer
	QSharedPointer<CVertexBuffer> m_filledPolygonBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw filled polygons

	QOpenGLDebugLogger *m_pOpenGLLogger;	///< OpenGL error logger

	QSharedPointer<CMapShaderProgram> m_pMapShader;///shader

	std::vector<std::vector<GenericVertexData>>m_pLineData; ///< Vector where all lines are stored

	std::vector<GenericVertexData>m_pPointData; ///< Vector where all points are stored

	std::vector<std::vector<GenericVertexData>>m_pPolygonData; ///< Vector where all polygons and their points are stored

	std::vector<std::vector<GenericVertexData>>m_pCircleData;///< Vector where circles and their points are stored

	void logOpenGLErrors();

	void addPointstoBuffer();

	void drawMultipleLines();

	int drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer ,const std::vector<std::vector<GenericVertexData>> &data);
};

#endif // CUSERMAPSRENDERER_H
