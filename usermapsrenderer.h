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
#include "usermapsvertexdata.h"
#include <vector>
#include "../UserMapsDataLib/usermap.h"
#include "../UserMapsDataLib/UserMapObjects/usermappoint.h"
#include "../UserMapsDataLib/UserMapObjects/usermaparea.h"
#include "../UserMapsDataLib/UserMapObjects/usermapcircle.h"
#include "../UserMapsDataLib/UserMapObjects/usermapline.h"
#include "../UserMapsDataLib/UserMapObjects/usermapobject.h"
#include "../UserMapsDataLib/usermaplinestyle.h"
#include "../ShipDataLib/shipdata.h"
#include "../LayerLib/viewcoordinates.h"
#include "../LayerLib/corelayer.h"

////////////////////////////////////////////////////////////////////////////////
///
///  \brief	This class is used for saving received texture data
///
////////////////////////////////////////////////////////////////////////////////
struct MapPoint
{
	MapPoint();
	GenericVertexData m_vertexData;		///< Position and colour of the point
	float m_iconSize;			    ///< Size of an icon.
	int m_icon;
};


////////////////////////////////////////////////////////////////////////////////
///
///  \brief	This class implements CUserMapsRenderer class which renders targets
///
////////////////////////////////////////////////////////////////////////////////
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
	void updateLines( const QMap<int, QSharedPointer<CUserMapLine> >& loadedLines);
	void updateLine( const QSharedPointer<CUserMapLine> & it, qreal originX, qreal originY, qreal offsetX, qreal offsetY);
	void updateCircles( const QMap<int, QSharedPointer<CUserMapCircle> >& loadedCircles);
	void updateCircle( const QSharedPointer<CUserMapCircle>& it, std::vector<GenericVertexData>& circle, qreal originX, qreal originY, qreal offsetX, qreal offsetY);
	void fillCircle( const std::vector<GenericVertexData>& circle, qreal originX, qreal originY, QVector4D colour);
	void updatePolygons( const QMap<int, QSharedPointer<CUserMapArea> >& loadedAreas);
	void updatePolygon( const QSharedPointer<CUserMapArea>& it,  std::vector<GenericVertexData>& polygon, qreal originX, qreal originY, qreal offsetX, qreal offsetY);
	void fillPolygon( const std::vector<GenericVertexData>& polygon, QVector4D colour);
	void updatePointsData( const QMap<int, QSharedPointer<CUserMapPoint> > &uPointData);
	void updatePointData( const QSharedPointer<CUserMapPoint>& it, qreal originX, qreal originY, qreal offsetX, qreal offsetY);

	// Draws
	void drawPoints( QOpenGLFunctions* func );
	void drawLines( QOpenGLFunctions* func );
	void drawCircles( QOpenGLFunctions* func );
	void drawfilledCircles(QOpenGLFunctions* func );
	void drawPolygons( QOpenGLFunctions* func );
	void drawfilledPolygons( QOpenGLFunctions* func );
	void initShader();
	void addText( QString text, double x, double y, QVector4D colour, TextAlignment alignment);
	void loadMaps();

private:
	QVector4D m_PointColour;					///< Point colour
	QVector4D m_LineColour;					    ///<Line colour
	QVector4D m_CircleColour;					///<Circle colour
	QVector4D m_PolygonColour;					///<Polygon colour
	QVector4D m_TextColour;					    ///<Text colour
	CStringRenderer	m_tgtTextRenderer;	    ///<Used for rendering text
	QSharedPointer<CVertexBuffer> m_PointBuf;	///< OpenGL vertex buffer (vertices and colour) to draw points
	std::vector<QSharedPointer<CImageTexture>> m_pTexture;  ///< image used as a textures

	// Lines buffer
	QSharedPointer<CVertexBuffer> m_LineBuf;	///< VBO used to draw Lines

	//circle buffer
	QSharedPointer<CVertexBuffer> m_CircleBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw circles

	//circle buffer
	QSharedPointer<CVertexBuffer> m_InlineCircleBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw filled circle

	//polygon buffer
	QSharedPointer<CVertexBuffer> m_PolygonBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw polygons

	//filled polygon buffer
	QSharedPointer<CVertexBuffer> m_filledPolygonBuf;  ///< OpenGL vertex buffer (vertices and colour) to draw filled polygons

	QOpenGLDebugLogger *m_pOpenGLLogger;	///< OpenGL error logger

	QSharedPointer<CMapShaderProgram> m_pMapShader;///shader

	std::vector<CUserMapsVertexData> m_pLineData; ///< Vector where all lines are stored

	std::vector<GenericVertexData> m_pPointData; ///< Vector where all points are stored

	std::vector<CUserMapsVertexData> m_pPolygonData; ///< Vector where all polygons and their points are stored

	std::vector<CUserMapsVertexData> m_pCircleData;///< Vector where circles and their points are stored

	std::vector<std::vector<GenericVertexData>> m_pfilledCircleData;///< Vector where circles and their points with inline colour are stored

	std::vector<std::vector<GenericVertexData>> m_pfilledPolygonData;///< Vector where polygons and their points with inline colour are stored

	std::vector<MapPoint> m_pPoints;			///< vector whose elements are lists of point objects contained in the map.

	void logOpenGLErrors();

	void addPointstoBuffer();

	void drawMultipleLines();

	int drawMultipleElements( QSharedPointer<CVertexBuffer> &buffer, const std::vector<std::vector<GenericVertexData>> &data);
	int drawMultipleElements( QSharedPointer<CVertexBuffer> &buffer, const std::vector<CUserMapsVertexData> &data);

	void testCircle( qreal originX, qreal originY);

	void read( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, QOpenGLFunctions *func);
	QVector4D convertColour( int colourKey, float opacity = 1.0f);

	void setLineStyle( CUserMapsVertexData& tempData, EUserMapLineStyle lineStyle, float lineWidth);

	QTextStream out;

};

#endif // CUSERMAPSRENDERER_H
