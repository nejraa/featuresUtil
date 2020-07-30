////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsrenderer.cpp
///
///	\author Elreg
///
///	\brief	Implementation of the CUserMapsRenderer class which renders
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#include "usermapsrenderer.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QOpenGLFramebufferObject>
#include "../OpenGLBaseLib/genericvertexdata.h"
#include <QTextStream>
#include "../UserMapsDataLib/usermapsmanager.h"
#include "../LoggingLib/logginglib.h"
#include "../UserMapsDataLib/usermapcolourmanager.h"
#include "../UserMapsDataLib/usermapiconmanager.h"


#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69 ///<taken from opengl specifications
#endif

const bool LOG_OPENGL_ERRORS = false; ///< Used for open GL errors.
const int rbDegrees = 360; ///< A circle has 360 degrees.
static const int FONT_PT_SIZE = 20; ///< Font size.

MapPoint::MapPoint()
	: m_vertexData(QVector4D( 0.0f, 0.0f, 0.0f, 0.0f ), QVector4D(0.0f , 0.0f, 0.0f, 0.0f)),
	  m_iconSize(0.0f),
	  m_icon(0)
{

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::CUserMapsRenderer()
///
/// \brief  Constructor.
////////////////////////////////////////////////////////////////////////////////
CUserMapsRenderer::CUserMapsRenderer()
	: CBaseRenderer("UserMapsView", OGL_TYPE::PROJ_ORTHO),
	  m_tgtTextRenderer(TextRendering::OPENGL),
	  m_PointBuf(nullptr),
	  m_LineBuf(nullptr),
	  m_CircleBuf(nullptr),
	  m_PolygonBuf(nullptr),
	  m_pOpenGLLogger(nullptr),
	  m_pMapShader(nullptr),
	  out(stdout)
{
}

////////////////////////////////////////////////////////////////////////////////
/// \fn CUserMapsRenderer::~CUserMapsRenderer()
///
/// \brief Destructor.
////////////////////////////////////////////////////////////////////////////////
CUserMapsRenderer::~CUserMapsRenderer()
{
	m_tgtTextRenderer.clearText();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CCUserMapsRenderer::render()
///
/// \brief	Called by the Qt framework to render shapes and text.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::render()
{
	framebufferObject()->bind();
	QOpenGLFunctions* pFunctions = QOpenGLContext::currentContext()->functions();

	// Clear the FBO to transparent black
	pFunctions->glClearColor( 0, 0, 0, 0 );
	pFunctions->glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

	// Enable alpha blending
	pFunctions->glEnable (GL_BLEND );

	//enable fixed restart index so number of draw calls will be greatly reduced,and performance will be improved
	pFunctions->glEnable ( GL_PRIMITIVE_RESTART_FIXED_INDEX);
	pFunctions->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	// Set The Blending Function For Translucency

	renderPrimitives( pFunctions );
	renderTextures();

	// Disable blending after use
	pFunctions->glDisable( GL_BLEND );

	framebufferObject()->release();

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CCUserMapsRenderer::initShader()
///
/// \brief	Called by the Qt framework to initialize map shader.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::initShader()
{
	if( m_pMapShader == nullptr )
		m_pMapShader = QSharedPointer<CMapShaderProgram>(new CMapShaderProgram());

}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::synchronize(QQuickFramebufferObject *item)
///
/// \brief	Called by the Qt framework to synchronise data with the Layer.
///
/// \param	item - UserMapslayer to synchronise with.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::synchronize(QQuickFramebufferObject *item)
{
	Q_UNUSED(item);

	// Get the view dimensions
	qreal left = 0, right = 0, top = 0, bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );


	// Initialise OpenGL if needed
	if(!m_bGLinit)
	{
		initializeGL();
	}

	m_tgtTextRenderer.clearText();

	float pixelsInMm = static_cast<float>(CViewCoordinates::Instance()->getScreenMmToPixels());
	if ( pixelsInMm == 0.0f )
		return;

	m_pPoints.clear();
	m_pTexture.clear();
	m_pfilledPolygonData.clear();
	m_pCircleData.clear();
	m_pfilledCircleData.clear();
	m_pLineData.clear();
	m_pPolygonData.clear();

	// pick selected objects
	const QMap<QString, QSharedPointer<CUserMap> > &loadedMaps = CUserMapsManager::getLoadedMapsStat();
	QMap<QString, QSharedPointer<CUserMap>>::const_iterator iter = loadedMaps.constBegin();
	while (iter != loadedMaps.constEnd())
	{
		auto &pMap = iter.value();
		const CUserMapObjectContainer<CUserMapPoint> &points = pMap->getPoints();
		const CUserMapObjectContainer<CUserMapArea> &areas = pMap->getAreas();
		const CUserMapObjectContainer<CUserMapLine> &lines = pMap->getLines();
		const CUserMapObjectContainer<CUserMapCircle> &circles = pMap->getCircles();

		for (auto item : {EUserMapObjectStatus::Loaded, EUserMapObjectStatus::Edited, EUserMapObjectStatus::Created})
		{
			updatePointsData(points.map(item));
			updateLines(lines.map(item));
			updateCircles(circles.map(item));
			updatePolygons(areas.map(item));
		}

		EUserMapObjectType objType = pMap->getSelectedObjectType();

		switch (objType)
		{
		case EUserMapObjectType::Point: {
			updatePointData(pMap->getSelectedObject().staticCast< CUserMapPoint>());
			break;
		}
		case EUserMapObjectType::Circle:
		{
			std::vector<GenericVertexData> circle;
			updateCircle(pMap->getSelectedObject().staticCast< CUserMapCircle>(), circle);
			circle.pop_back();//remove last point, because it is same as the first one

			fillCircle(circle, convertColour(pMap->getSelectedObject().staticCast< CUserMapCircle>()->getColor(), pMap->getSelectedObject().staticCast< CUserMapCircle>()->getTransparency()));
			break;
		}
		
		case EUserMapObjectType::Line:
		{
			updateLine(pMap->getSelectedObject().staticCast< CUserMapLine>());
			break;

		}
		
		case EUserMapObjectType::Area:
		{
			std::vector<GenericVertexData> area;
			updatePolygon(pMap->getSelectedObject().staticCast< CUserMapArea>(), area);
			fillPolygon(area, convertColour(pMap->getSelectedObject().staticCast< CUserMapArea>()->getColor() , pMap->getSelectedObject().staticCast< CUserMapArea>()->getTransparency()));
			break;
		}
		case EUserMapObjectType::Unkown_Object:
		{
		}
		}

		pMap->getSelectedObject();

		iter++;
	}

	// Set the width, height and projection for each target texture
	for( uint i = 0; i < m_pTexture.size(); ++i )
	{
		float imgWidthInMM = m_pTexture[i]->imageWidth() / 20.0f;	// Images are designed to be 20 texels/mm
		float textureWidthInPixels = imgWidthInMM * pixelsInMm;	// Total width in pixels
		float imgHeightInMM = m_pTexture[i]->imageHeight() / 20.0f;
		float textureHeightInPixel = imgHeightInMM * pixelsInMm;
		m_pTexture[i]->setWidth(textureWidthInPixels/2.0f); // set width for one side (left/right)
		m_pTexture[i]->setHeight(textureHeightInPixel/2.0f);
		m_pTexture[i]->setProjection(left, right, bottom, top);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::initializeGL()
///
/// \brief	Initialise OpenGL for this renderer.
///			Includes creating the image textures;
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::initializeGL()
{
	QOpenGLFunctions* func = QOpenGLContext::currentContext()->functions();
	func->initializeOpenGLFunctions();

	func->glClearColor(0, 0, 0, 0);
	func->glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	if( m_pOpenGLLogger == nullptr )
	{
		QOpenGLContext *pContext = QOpenGLContext::currentContext();
		if( pContext != nullptr )
		{
			if( LOG_OPENGL_ERRORS )
			{
				if( pContext->hasExtension(QByteArrayLiteral("GL_KHR_debug")) )
				{
					m_pOpenGLLogger = new QOpenGLDebugLogger();
					bool bOk = m_pOpenGLLogger->initialize();
					if ( bOk )
						qDebug() << "\nCUserMapsRenderer OpenGL debug logging initialized";
					else
						qDebug() << "\nCUserMapsRenderer OpenGL debug logging initialization failed";
				}
			}
		}
	}

	// Initialise the string renderer
	QOpenGLFunctions* openGL = QOpenGLContext::currentContext()->functions();

	if( openGL->glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE )
	{
		m_tgtTextRenderer.init( framebufferObject(), &m_textureShader );

		// Get the view dimensions
		qreal left = 0;
		qreal right = 0;
		qreal top = 0;
		qreal bottom = 0;
		CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );
		m_tgtTextRenderer.setScreenGeometry( QRect( static_cast<int> ( left ),
													static_cast<int> ( top ),
													static_cast<int> ( right ),
													static_cast<int> ( bottom ) ) );

	}
	m_bGLinit = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::renderPrimitives( QOpenGLFunctions* func )
///
/// \brief	Render shapes.
///
/// \param	func - The QOpenGLFunctions to use if needed.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::renderPrimitives(QOpenGLFunctions *func)
{

	drawPoints(func);
	drawfilledPolygons(func);
	drawfilledCircles(func);
	drawLines(func);
	drawCircles(func);
	drawPolygons(func);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::renderTextures()
///
/// \brief	Render the textures and text.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::renderTextures()
{
	m_textureShader.bind();

	for ( uint i = 0; i < m_pPoints.size(); ++i )
	{
		// Calculate target plot data
		QMatrix4x4 matrix;

		// Set translation
		matrix.translate(m_pPoints[i].m_vertexData.position().x(),m_pPoints[i].m_vertexData.position().y(), 0.0f );

		// Set scale
		float fScaleWidth = m_pTexture[i]->getWidth();
		float fScaleHeight = m_pTexture[i]->getHeight();
		matrix.scale(fScaleWidth, fScaleHeight, 0.0f);

		// Set the projection matrix
		m_textureShader.setMVPMatrix(m_pTexture[i]->getProjection() * matrix);

		// Set the texture colour
		m_textureShader.setTexUserColour(m_pPoints[i].m_vertexData.color());

		// Use texture unit 0 for the sampler
		m_textureShader.setTextureSampler(0);

		// Draw the target
		m_pTexture[i]->drawTexture(&m_textureShader);
	}

	m_tgtTextRenderer.renderText();
	m_textureShader.release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updateLines(const QMap<int, QSharedPointer<CUserMapLine> >&loadedLines)
///
/// \brief	Add line points so line could be drawn.
///
/// \param	loadedLines- lines that should be drawn.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateLines(const QMap<int, QSharedPointer<CUserMapLine> >&loadedLines )
{
	if( !m_LineBuf.isNull() )
	{
		m_LineBuf.clear();
		m_LineBuf = nullptr;
	}

	if(loadedLines.empty()) return; //if there is not any line return

	// Screen information
	double pixelsInMm = CViewCoordinates::Instance()->getScreenMmToPixels();
	if ( pixelsInMm == 0.0 )
		return ;//if screen information cannot be read also return;

	for(QMap<int, QSharedPointer<CUserMapLine> >::const_iterator it = loadedLines.constBegin(); it != loadedLines.constEnd() ; it++)
	{
		std::vector<GenericVertexData> line;
		updateLine(it.value());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updateLine(const QSharedPointer<CUserMapLine>& it)
///
/// \brief	Add points so line could be drawn.
///
/// \param	it - Pointer that points to line.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateLine(const QSharedPointer<CUserMapLine>& it)
{
	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );


	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	std::vector<GenericVertexData> line;
	CUserMapsVertexData tempData;
	for (const CPosition & point : it->getPoints())
	{

		// Relative target position (in pixels) from the ownship (Geo Origin)
		GEOGRAPHICAL dfLat = ToGEOGRAPHICAL(point.Latitude());
		GEOGRAPHICAL dfLon = ToGEOGRAPHICAL(point.Longitude());
		PIXEL tgtPosX;
		PIXEL tgtPosY;
		CViewCoordinates::Instance()->Convert(dfLat, dfLon, tgtPosX, tgtPosY);

		// Absolute target position (in pixels)
		double xPos = tgtPosX + originX;
		double yPos = tgtPosY + originY;

		line.push_back( GenericVertexData(QVector4D( static_cast<float>(xPos), static_cast<float>(yPos), 0.0f, 1.0f), convertColour(it->getColor(), it->getTransparency())));
	}

	tempData.setVertexData(line);
	setLineStyle(tempData,it->getLineStyle(),it->getLineWidth());

	m_pLineData.push_back(tempData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updateCircles(const QMap<int, QSharedPointer<CUserMapCircle> >& loadedCircles)
///
/// \brief	Add Circle points so circle could be drawn.
///
/// \param	loadedCircles - Circles that should be drawn.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateCircles(const QMap<int, QSharedPointer<CUserMapCircle> >& loadedCircles)
{

	if( !m_CircleBuf.isNull() )
	{
		m_CircleBuf.clear();
		m_CircleBuf = nullptr;
	}

	if(loadedCircles.empty())
		return;

	// Screen information
	double pixelsInMm = CViewCoordinates::Instance()->getScreenMmToPixels();
	if ( pixelsInMm == 0.0 )
		return ;

	for (QMap<int, QSharedPointer<CUserMapCircle> >::const_iterator it = loadedCircles.constBegin(); it != loadedCircles.constEnd() ; it++)
	{
		std::vector<GenericVertexData> circle;
		updateCircle(it.value(), circle );
		circle.pop_back();//remove last point, because it is same as the first one

		fillCircle(circle, convertColour( it.value()->getColor(),it.value()->getTransparency()));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updateCircle(const QSharedPointer<CUserMapCircle>& it, std::vector<GenericVertexData>& circle)
///
/// \brief	Add Circle points so circle could be drawn.
///
/// \param	it - Pointer that points to circle.
///         circle - Vector where circle points will be stored.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateCircle(const QSharedPointer<CUserMapCircle>& it, std::vector<GenericVertexData>& circle)
{
	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	const int k = 8; // k is used as a circle segment for  drawing circle
	circle.reserve(360 / k + 3);//number of points needed for circle

	double radius = it->getRadius() * CViewCoordinates::getNauticalMilesToPixels();

	int bufferIndex = 0;

	// Relative target position (in pixels) from the ownship (Geo Origin)
	GEOGRAPHICAL dfLat = ToGEOGRAPHICAL(it->getCenter().Latitude());
	GEOGRAPHICAL dfLon = ToGEOGRAPHICAL(it->getCenter().Longitude());
	PIXEL tgtPosX;
	PIXEL tgtPosY;
	CViewCoordinates::Instance()->Convert(dfLat, dfLon, tgtPosX, tgtPosY);

	// Absolute target position (in pixels)
	double xCenter = tgtPosX + originX;
	double yCenter = tgtPosY + originY;

	// Draw the circle (line strip)
	for ( bufferIndex = 0; bufferIndex <= rbDegrees; bufferIndex += k )
	{
		double angle = 2 * M_PI * bufferIndex / rbDegrees;
		float x = static_cast<float>(xCenter + (radius * std::sin(angle)));
		float y = static_cast<float>(yCenter - (radius * std::cos(angle)));

		// Set Vertex data
		circle.push_back( GenericVertexData(QVector4D(x, y, 0.0f, 1.0f), convertColour(it->getOutlineColor())));

		// Check if it is the last point
		if( bufferIndex == rbDegrees )
		{
			circle.push_back( GenericVertexData(QVector4D(x, y, 0.0f, 1.0f), convertColour(it->getOutlineColor())));
		}

	}
	CUserMapsVertexData tempData;
	tempData.setVertexData(circle);
	setLineStyle(tempData,it->getLineStyle(),it->getLineWidth());

	m_pCircleData.push_back(tempData);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::fillCircle(const std::vector<GenericVertexData>& circle, qreal originX, qreal originY, QVector4D colour)
///
/// \brief	Add Circle points so circle could be drawn.
///
/// \param	it - Pointer that points to circle.
///         circle - Vector where circle points will be stored.
///         colour - Used to paint circle.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::fillCircle(const std::vector<GenericVertexData>& circle, QVector4D colour)
{
	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	std::vector<GenericVertexData> filledCircle;
	for(const GenericVertexData & data : circle)
	{
		filledCircle.push_back(GenericVertexData(data.position(), colour));
	}
	filledCircle.push_back(GenericVertexData(QVector4D(originX, originY, 0.0f, 1.0f), colour));// add center so,circles could be drawn more effectively in opengl
	m_pfilledCircleData.push_back(filledCircle);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updatePolygons(const QMap<int, QSharedPointer<CUserMapArea> >& loadedArea)
///
/// \brief	Adds polygon points.
///
/// \param	loadedArea - Received areas that should be drawn.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePolygons(const QMap<int, QSharedPointer<CUserMapArea> >& loadedAreas)
{

	if( !m_PolygonBuf.isNull() )
	{
		m_PolygonBuf.clear();
		m_PolygonBuf = nullptr;
	}

	if(loadedAreas.empty())
		return;

	// Screen information
	double pixelsInMm = CViewCoordinates::Instance()->getScreenMmToPixels();
	if ( pixelsInMm == 0.0 )
		return ;

	QMap<int, QSharedPointer<CUserMapCircle> >::Iterator it;

	for (QMap<int, QSharedPointer<CUserMapArea> >::const_iterator it = loadedAreas.constBegin(); it != loadedAreas.constEnd() ; it++)
	{
		std::vector<GenericVertexData> polygon; //test case polygon ,will be deleted

		updatePolygon(it.value(), polygon);
		fillPolygon(polygon, convertColour(it.value()->getColor(), it.value()->getTransparency()));

	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updatePolygon(const QSharedPointer<CUserMapArea>& it, std::vector<GenericVertexData>& polygon)
///
/// \brief	Add area points so polygon could be drawn.
///
/// \param	it - Pointer that points to area.
///         polygon - Vector where polygon points will be stored.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePolygon(const QSharedPointer<CUserMapArea>& it, std::vector<GenericVertexData>& polygon)
{
	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	for(const CPosition & point : it->getPoints())
	{

		// Relative target position (in pixels) from the ownship (Geo Origin)
		GEOGRAPHICAL dfLat = ToGEOGRAPHICAL(point.Latitude());
		GEOGRAPHICAL dfLon = ToGEOGRAPHICAL(point.Longitude());
		PIXEL tgtPosX;
		PIXEL tgtPosY;
		CViewCoordinates::Instance()->Convert(dfLat, dfLon, tgtPosX, tgtPosY);

		// Absolute target position (in pixels)
		double xPos = tgtPosX + originX;
		double yPos = tgtPosY + originY;

		polygon.push_back( GenericVertexData(QVector4D( static_cast<float>(xPos), static_cast<float>(yPos), 0.0f, 1.0f), convertColour(it->getOutlineColor())));
	}

	CUserMapsVertexData tempData;
	tempData.setVertexData(polygon);
	setLineStyle(tempData, it->getLineStyle(), it->getLineWidth());
	m_pPolygonData.push_back(tempData);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::fillPolygon(const std::vector<GenericVertexData> &polygon,  QVector4D colour)
///
/// \brief	Add polygon points so filled polygon could be drawn.
///
/// \param	it - pointer that points to circle.
///         polygon - vector where polygon points will be stored.
///         colour - used to paint polygon.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::fillPolygon(const std::vector<GenericVertexData> &polygon,  QVector4D colour)
{
	std::vector<GenericVertexData>result;
	Triangulate::Process(polygon, result); //triangulate received points

	for(GenericVertexData &data : result)
		data.setColor(colour);
	m_pfilledPolygonData.push_back(result);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updatePointsData(const QMap<int, QSharedPointer<CUserMapPoint> > &pointData)
///
/// \brief	Add textures that should be drawn.
///
/// \param	pointData - Textures details.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePointsData(const QMap<int, QSharedPointer<CUserMapPoint> > &pointData)
{

	for(const QSharedPointer<CUserMapPoint>& uPoint : pointData)
	{
		updatePointData(uPoint);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::updatePointData(const QSharedPointer<CUserMapPoint>& uPoint)
///
/// \brief	Updates points so they could be drawn.
///
/// \param	uPoint - Pointer that points to UserMapPoint.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePointData(const QSharedPointer<CUserMapPoint>& uPoint)
{
	qreal originX = 0;
	qreal originY = 0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	// Geo Origin
	qreal offsetX = 0;
	qreal offsetY = 0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	MapPoint data;
	GEOGRAPHICAL dfLat = ToGEOGRAPHICAL(uPoint->getPosition().Latitude());
	GEOGRAPHICAL dfLon = ToGEOGRAPHICAL(uPoint->getPosition().Longitude());
	PIXEL tgtPosX;
	PIXEL tgtPosY;
	CViewCoordinates::Instance()->Convert(dfLat, dfLon, tgtPosX, tgtPosY);

	// Absolute target position (in pixels)
	double xPos = tgtPosX + originX;
	double yPos = tgtPosY + originY;

	QVector4D colour = convertColour(uPoint->getColor(),uPoint->getTransparency());
	// Set attributes

	data.m_icon = uPoint->getIcon();
	data.m_iconSize = uPoint->getIconSize();
	data.m_vertexData= GenericVertexData(QVector4D( static_cast<float>(xPos), static_cast<float>(yPos), 0.0f, 1.0f ),colour);

	QString strIconPath = CUserMapIconManager::instance()->getIconPath(data.m_icon);

	m_pPoints.push_back(data);
	m_pTexture.push_back( QSharedPointer<CImageTexture>(new CImageTexture( strIconPath, colour)));

}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::drawPoints(QOpenGLFunctions *func)
///
/// \brief  Handles mouse move event.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawPoints(QOpenGLFunctions *func)
{

	// Set translation matrix (no translation)
	QMatrix4x4 translation;
	translation.translate(0.0, 0.0, 0.0);

	// Set projection matrix
	QMatrix4x4 projection;
	qreal left = 0;
	qreal right = 0;
	qreal top = 0;
	qreal bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );
	setProjection( left, right, bottom, top, projection );

	// Bind the shader
	m_primShader.bind();

	// Set the projection and translation
	m_primShader.setMVPMatrix(projection * translation);

	addPointstoBuffer();

	m_PointBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawPoints() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_primShader.setupVertexState();

	func->glDrawArrays(GL_POINTS, 0, m_pPointData.size());
	// Tidy up
	m_primShader.cleanupVertexState();

	m_PointBuf->release();

	m_primShader.release();

}

////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::drawLines(QOpenGLFunctions *func)
///
/// \brief	Draws Lines.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawLines(QOpenGLFunctions *func)
{

	// Set translation matrix (no translation)
	QMatrix4x4 translation;
	translation.translate(0.0, 0.0, 0.0);

	// Set projection matrix
	QMatrix4x4 projection;
	qreal left = 0;
	qreal right = 0;
	qreal top = 0;
	qreal bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );
	setProjection( left, right, bottom, top, projection );

	initShader();

	// Bind the shader
	m_pMapShader->bind();

	// Set the projection and translation
	m_pMapShader->setMVPMatrix(projection * translation);

	GLfloat winWidth = static_cast<float>(right-left);
	GLfloat winHeight = static_cast<float>(bottom-top);

	m_pMapShader->setResolution(winWidth, winHeight);


	drawMultipleElements(m_LineBuf, m_pLineData);

	m_LineBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawLines() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_pMapShader->setupVertexState();

	int offset = 0;


	func->glLineWidth(500);

	for(uint i = 0; i<m_pLineData.size(); i++ )
	{
		m_pMapShader->setDashSize(m_pLineData[i].getDashSize());
		m_pMapShader->setGapSize(m_pLineData[i].getGapSize());
		m_pMapShader->setDotSize(m_pLineData[i].getDotSize());

		func->glDrawArrays(GL_LINE_STRIP, offset,  m_pLineData[i].getVertexData().size());
		func->glLineWidth(1);
		offset += m_pLineData[i].getVertexData().size();
	}
	//func->glDrawArrays(GL_LINE_STRIP, 0,  counter);
	// Tidy up
	m_pMapShader->cleanupVertexState();

	m_LineBuf->release();

	m_pMapShader->release();

}


////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::drawPolygons(QOpenGLFunctions *func)
///
/// \brief	Draws polygons.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawPolygons(QOpenGLFunctions *func)
{

	// Set translation matrix (no translation)
	QMatrix4x4 translation;
	translation.translate(0.0, 0.0, 0.0);

	// Set projection matrix
	QMatrix4x4 projection;
	qreal left = 0;
	qreal right = 0;
	qreal top = 0;
	qreal bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );
	setProjection( left, right, bottom, top, projection );

	initShader();
	m_pMapShader->bind();

	// Set the projection and translation
	m_pMapShader->setMVPMatrix(projection * translation);

	GLfloat winWidth = static_cast<float>(right-left);
	GLfloat winHeight = static_cast<float>(bottom-top);

	m_pMapShader->setResolution(winWidth, winHeight);

	drawMultipleElements(m_PolygonBuf, m_pPolygonData);

	m_PolygonBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawPolygons() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_pMapShader->setupVertexState();

	int offset = 0;

	for(uint i = 0; i<m_pPolygonData.size(); i++ )
	{
		m_pMapShader->setDashSize(m_pPolygonData[i].getDashSize());
		m_pMapShader->setGapSize(m_pPolygonData[i].getGapSize());
		m_pMapShader->setDotSize(m_pPolygonData[i].getDotSize());

		func->glDrawArrays(GL_LINE_LOOP, offset,  m_pPolygonData[i].getVertexData().size());
		func->glLineWidth(1);
		offset += m_pPolygonData[i].getVertexData().size();
	}

	// Tidy up
	m_pMapShader->cleanupVertexState();

	m_PolygonBuf->release();

	m_pMapShader->release();

}

////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::drawfilledPolygons(QOpenGLFunctions *func)
///
/// \brief  Draws polygons.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawfilledPolygons(QOpenGLFunctions *func)
{

	// Set translation matrix (no translation)
	QMatrix4x4 translation;
	translation.translate(0.0, 0.0, 0.0);

	// Set projection matrix
	QMatrix4x4 projection;
	qreal left = 0;
	qreal right = 0;
	qreal top = 0;
	qreal bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );
	setProjection( left, right, bottom, top, projection );

	// Bind the shader
	m_primShader.bind();

	// Set the projection and translation
	m_primShader.setMVPMatrix(projection * translation);

	int counter=drawMultipleElements(m_filledPolygonBuf, m_pfilledPolygonData);
	m_filledPolygonBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "COwnshipRenderer::drawPolygons() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_primShader.setupVertexState();

	//draw
	func->glDrawArrays(GL_TRIANGLES, 0, counter);

	// Tidy up
	m_primShader.cleanupVertexState();

	m_filledPolygonBuf->release();

	m_primShader.release();

}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::drawfilledCircles(QOpenGLFunctions *func)
///
/// \brief  Draws Circles.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawfilledCircles(QOpenGLFunctions *func) 
{
	// Set translation matrix (no translation)
	QMatrix4x4 translation;
	translation.translate(0.0, 0.0, 0.0);

	// Set projection matrix
	QMatrix4x4 projection;
	qreal left = 0;
	qreal right = 0;
	qreal top = 0;
	qreal bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top);
	setProjection( left, right, bottom, top, projection );

	// Bind the shader
	m_primShader.bind();

	// Set the projection and translation
	m_primShader.setMVPMatrix(projection * translation);


	// Tell OpenGL which VBOs to use
	drawMultipleElements(m_InlineCircleBuf, m_pfilledCircleData);
	m_InlineCircleBuf->bind();


	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawfilledCircles() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_primShader.setupVertexState();
	int offset = 0; //offset

	//Draw inline and outline circle from data in the VBOs

	for(uint  i = 0; i<m_pfilledCircleData.size(); i++)
	{
		func->glDrawArrays(GL_TRIANGLE_FAN, offset, m_pfilledCircleData[i].size());
		offset += m_pfilledCircleData[i].size();
	}
	func->glLineWidth( 1 );

	// Tidy up
	m_InlineCircleBuf->release();

	m_primShader.cleanupVertexState();

	m_primShader.release();

}


////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::drawCircles(QOpenGLFunctions *func)
///
/// \brief  Draws outline Circle.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawCircles(QOpenGLFunctions *func)
{
	// Set translation matrix (no translation)
	QMatrix4x4 translation;
	translation.translate(0.0, 0.0, 0.0);

	// Set projection matrix
	QMatrix4x4 projection;
	qreal left = 0;
	qreal right = 0;
	qreal top = 0;
	qreal bottom = 0;
	CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top);
	setProjection( left, right, bottom, top, projection );
	initShader();

	// Bind the shader
	m_pMapShader->bind();

	// Set the projection and translation
	m_pMapShader->setMVPMatrix(projection * translation);


	GLfloat winWidth = static_cast<float>(right-left);
	GLfloat winHeight = static_cast<float>(bottom-top);

	m_pMapShader->setResolution(winWidth, winHeight);

	// Tell OpenGL which VBOs to use
	drawMultipleElements(m_CircleBuf, m_pCircleData);
	m_CircleBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawCircles() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_pMapShader->setupVertexState();
	func->glLineWidth(1);

	int offset = 0;

	for(uint i = 0; i<m_pCircleData.size(); i++ )
	{
		m_pMapShader->setDashSize(m_pCircleData[i].getDashSize());
		m_pMapShader->setGapSize(m_pCircleData[i].getGapSize());
		m_pMapShader->setDotSize(m_pCircleData[i].getDotSize());

		func->glDrawArrays(GL_LINE_STRIP, offset,  m_pCircleData[i].getVertexData().size());
		func->glLineWidth(1);
		offset += m_pCircleData[i].getVertexData().size();
	}

	// Tidy up
	m_pMapShader->cleanupVertexState();

	m_CircleBuf->release();

	m_pMapShader->release();
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::addPointstoBuffer()
///
/// \brief	Adds points from vector to buffer
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::addPointstoBuffer() 
{
	m_PointBuf = QSharedPointer<CVertexBuffer>( new CVertexBuffer(m_pPointData.data(), m_pPointData.size()));
}


////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer ,
///								const std::vector<std::vector<GenericVertexData>> &data)
///
/// \brief  add genericvertexdata to buffer
///
/// \param  buffer - vertex buffer
///         data- points that form a shape
////////////////////////////////////////////////////////////////////////////////
int CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer,
							const std::vector<std::vector<GenericVertexData>> &data) 
{
	uint counter = 0;

	for( uint  i = 0; i < data.size(); i++)
	{
		counter += data[i].size();
	}

	std::vector<GenericVertexData> vertices;
	vertices.reserve(counter);

	for( uint  i=0; i < data.size(); i++)
	{
		for( uint j=0; j < data[i].size(); j++)
		{
			vertices.push_back(data[i][j]);
		}
	}
	buffer = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices.data(), counter));

	return counter;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn	int CUserMapsRenderer::drawMultipleElements( QSharedPointer<CVertexBuffer> &buffer,
///											const std::vector<CUserMapsVertexData> &data)
///
/// \brief	Adds usermapsvertexdata to buffer.
///
/// \param  buffer - Vertex buffer.
///         data - Points that form a shape.
///
/// \return Number of elements to be rendered.
////////////////////////////////////////////////////////////////////////////////
int CUserMapsRenderer::drawMultipleElements( QSharedPointer<CVertexBuffer> &buffer, 
											const std::vector<CUserMapsVertexData> &data)
{
	uint totalSize = 0;
	for (uint i = 0; i < data.size(); i++)
		totalSize += data[i].getVertexData().size();

	std::vector<GenericVertexData> vertices;
	vertices.reserve(totalSize);

	for (uint i = 0; i < data.size(); i++)
	{
		for (uint j = 0; j < data[i].getVertexData().size(); j++)
			vertices.push_back(data[i].getVertexData().at(j));
	}

	buffer = QSharedPointer<CVertexBuffer>(new CVertexBuffer( vertices.data(), totalSize));

	return totalSize;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::logOpenGLErrors()
///
/// \brief  OpenGL logger.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::logOpenGLErrors()
{
	if( m_pOpenGLLogger )
	{
		const QList<QOpenGLDebugMessage> messages = m_pOpenGLLogger->loggedMessages();
		for (const QOpenGLDebugMessage &message : messages)
		{
			//if( message.severity() == QOpenGLDebugMessage::HighSeverity || message.severity() == QOpenGLDebugMessage::MediumSeverity )
			qDebug() << "CUserMapsRenderer:" << message;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::addText( QString text, double x, double y, QVector4D colour,
///																TextAlignment alignment)
///
/// \brief	This function is used for writing text.
///
/// \param  text - Text to be written.
///			x - xAxis position of text.
///			y - yAxis position of text.
///			colour - Colour of the text.
///			alignment - Specifies text alignment.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::addText( QString text,double x, double y, QVector4D colour,
												TextAlignment alignment)
{
	m_tgtTextRenderer.addText( text, static_cast<int> (x ), static_cast<int> ( y ), FONT_PT_SIZE, colour, alignment );
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::testCircle(qreal originX, qreal originY)
///
/// \brief	This function is used for measuring drawing speed.
///
/// \param	originX - xAxis position of the origin.
///         originY - yAxis position of the origin.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::testCircle(qreal originX, qreal originY) 
{
	double radius = CViewCoordinates::Instance()->getRadiusPixels() / 128;

	int bufferIndex = 0;
	std::vector<GenericVertexData> circle;
	std::vector<GenericVertexData> circle2;
	std::vector<GenericVertexData> circle3;

	// Draw the circle (line strip)
	for ( bufferIndex = 0 ; bufferIndex < rbDegrees; bufferIndex += 8 ) //if circle is not round enough use ++ instead of 8
	{
		double angle = 2 * M_PI * bufferIndex / rbDegrees;
		float x = static_cast<float>(originX + (radius * std::sin(angle)));
		float y = static_cast<float>(originY - (radius * std::cos(angle)));

		// Set Vertex data
		circle.push_back(GenericVertexData(QVector4D( x, y, 0.0f, 1.0f ), QVector4D(1.0f ,0.0f , 0.0f, 1.0f)));

		// Check if it is the last point
	}
	m_pfilledCircleData.push_back(circle);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn void CUserMapsRenderer::read( GLint x, GLint y, GLsizei width, GLsizei height,
///							GLenum format, GLenum type, QOpenGLFunctions *func)
///
/// \brief	Reads colour data of the pixel.
///
/// \param	x - xAxis coordinate of the first pixel.
///			y - yAxis coordinate of the first pixel.
///			width - Width of the pixel rectangle, should be 1.
///			height - Height of the pixel rectangle, should be 1.
///			format - Format of the pixel data.
///			type - Data type of the pixel data.
///			func - Pointer that points to qopengl functions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::read( GLint x, GLint y, GLsizei width, GLsizei height,
								GLenum format, GLenum type, QOpenGLFunctions *func)
{
	func->glFlush();
	func->glFinish();
	func->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned char data[4] = { 0, 0, 0, 0 };

	func->glReadPixels( x, y, width, height, format, type, data );
	CLoggingLib::logging( E_DEBUGGING )<<endl<<"Colours of<<"<<x<<"and"<<y<<"are r:" <<data[0] << " g" <<data[1]<<" b" <<data[2] <<" a" << data[3];

}


////////////////////////////////////////////////////////////////////////////////
/// \fn QVector4D CUserMapsRenderer::convertColour(int colourKey, float opacity)
///
/// \brief  This function is used for converting colour into 4d vector.
///
/// \param	colourKey - Colour that should be converted.
///			opacity - opacity.
///
/// \return	4D Qvector of converted colour.
////////////////////////////////////////////////////////////////////////////////
QVector4D CUserMapsRenderer::convertColour(int colourKey, float opacity)
{
	QColor colour = CUserMapColourManager::instance()->getColourByKey(colourKey);
	return QVector4D(colour.red() / 255.0f, colour.green() / 255.0f, colour.blue() / 255.0f, opacity);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn	void CUserMapsRenderer::setLineStyle(CUserMapsVertexData& tempData, EUserMapLineStyle lineStyle, float lineWidth)
///
/// \brief	This function is used for setting up linestyles.
///
/// \param	tempData - Data to be drawn.
///			lineStyle - Enumerated lineStyle.
///			lineWidth - Width of the line.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::setLineStyle(CUserMapsVertexData& tempData, EUserMapLineStyle lineStyle, float lineWidth)
{
	switch (lineStyle)
	{
	case EUserMapLineStyle::Solid :
	{
		tempData.setDashSize(30.0f);
		tempData.setDotSize(0.0f);
		tempData.setGapSize(0.0f);
		break;
	}
	
	case EUserMapLineStyle::Dashed :
	{
		tempData.setDashSize(15.0f);
		tempData.setDotSize(0.0f);
		tempData.setGapSize(15.0f);
		break;
	}
	
	case EUserMapLineStyle::Dotted :
	{
		tempData.setDashSize(2.0f);
		tempData.setGapSize(10.0f);
		tempData.setDotSize(0.0f);
		break;
	}
	
	case EUserMapLineStyle::Dot_Dash :
	{
		tempData.setDashSize(30.0f);
		tempData.setGapSize(15.0f);
		tempData.setDotSize(10.0f);
		break;
	}
	}
	tempData.setLineWidth(lineWidth);
}


