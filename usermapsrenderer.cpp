////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsrenderer.cpp
///
///	\author Elreg
///
///	\brief	Definition of the CUserMapsRenderer class which renders
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2019.
////////////////////////////////////////////////////////////////////////////////

#include "usermapsrenderer.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QOpenGLFramebufferObject>
#include "../OpenGLBaseLib/genericvertexdata.h"
#include <QTextStream>
#include "../UserMapsDataLib/usermapsmanager.h"
#include "../LoggingLib/logginglib.h"


#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69 ///<taken from opengl specifications
#endif
const bool LOG_OPENGL_ERRORS = false; ///<used for open gl errors
const int rbDegrees = 360; ///< a circle has 360 degrees
static const int FONT_PT_SIZE = 20; ///< font

MapPoint::MapPoint()
	: m_vertexData(QVector4D( 0.0f, 0.0f, 0.0f, 0.0f ), QVector4D(0.0f , 0.0f, 0.0f, 0.0f)),
	  m_iconSize(0.0f),
	  m_icon(0)
{

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::CUserMapsRenderer()
///
/// \brief  Constructor
///
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
/// \brief Destructor
///
////////////////////////////////////////////////////////////////////////////////
CUserMapsRenderer::~CUserMapsRenderer()
{
	m_tgtTextRenderer.clearText();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CCUserMapsRenderer::render()
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
/// \fn	CCUserMapsRenderer::initShader()
///
/// \brief	Called by the Qt framework to initialize map shader
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::initShader()
{
	if( m_pMapShader == nullptr )
		m_pMapShader = QSharedPointer<CMapShaderProgram>(new CMapShaderProgram());

}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CUserMapsRenderer::synchronize(QQuickFramebufferObject *item)
///
/// \brief	Called by the Qt framework to synchronise data with the Layer.
///
/// \param	item	UserMapslayer to synchronise with.
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

	m_pfilledPolygonData.clear();
	m_pCircleData.clear();
	m_pfilledCircleData.clear();
	m_pLineData.clear();
	m_pPolygonData.clear();

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
/// \fn	CUserMapsRenderer::initializeGL()
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
/// \fn	CUserMapsRenderer::renderPrimitives( QOpenGLFunctions* func )
///
/// \brief	Render shapes.
///
/// \param	func	The QOpenGLFunctions to use if needed.
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
/// \fn	CUserMapsRenderer::renderTextures()
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
/// \fn	CUserMapsRenderer::updateLines(const QMap<int, QSharedPointer<CUserMapLine> >&loadedLines)
///
/// \brief	Add line points so line could be drawn
///
/// \param loadedLines- lines that should be drawn
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

	std::vector<GenericVertexData> line;

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );


	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );

	for(QMap<int, QSharedPointer<CUserMapLine> >::const_iterator it = loadedLines.constBegin(); it != loadedLines.constEnd() ; it++)
	{
		std::vector<GenericVertexData> line;
		for(const CPosition & point : it.value()->getPoints())
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

			line.push_back( GenericVertexData(QVector4D( static_cast<float>(xPos), static_cast<float>(yPos), 0.0f, 1.0f), convertColour(it.value()->getColor(), it.value()->getTransparency())));
		}

		CUserMapsVertexData tempData;
		tempData.setVertexData(line);
		setLineStyle(tempData,it.value()->getLineStyle(),it.value()->getLineWidth());

		m_pLineData.push_back(tempData);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CUserMapsRenderer::updateCircles(const QMap<int, QSharedPointer<CUserMapCircle> >& loadedCircles)
///
/// \brief	Add Circle points so circle could be drawn.
///
/// \param	loadedCircles-circles that should be drawn
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateCircles(const QMap<int, QSharedPointer<CUserMapCircle> >& loadedCircles) {

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

	//TODO AM: better name and/or explanation of the variable needed. If it does not change, declare it const.
	int k = 8;


	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );

	for (QMap<int, QSharedPointer<CUserMapCircle> >::const_iterator it = loadedCircles.constBegin(); it != loadedCircles.constEnd() ; it++)
	{
		std::vector<GenericVertexData> circle;
		circle.reserve(360 / k + 3);//number of points needed for circle

		double radius = it.value()->getRadius() * CViewCoordinates::getNauticalMilesToPixels();

		int bufferIndex = 0;

		// Relative target position (in pixels) from the ownship (Geo Origin)
		GEOGRAPHICAL dfLat = ToGEOGRAPHICAL(it.value()->getCenter().Latitude());
		GEOGRAPHICAL dfLon = ToGEOGRAPHICAL(it.value()->getCenter().Longitude());
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
			circle.push_back( GenericVertexData(QVector4D(x, y, 0.0f, 1.0f), convertColour(it.value()->getOutlineColor())));

			// Check if it is the last point
			if( bufferIndex == rbDegrees )
			{
				// Change to transparent colour at the same position
				bufferIndex++;
				circle.push_back( GenericVertexData(QVector4D(x, y, 0.0f, 1.0f), convertColour(it.value()->getOutlineColor())));

			}
		}
		CUserMapsVertexData tempData;
		tempData.setVertexData(circle);
		setLineStyle(tempData,it.value()->getLineStyle(),it.value()->getLineWidth());

		m_pCircleData.push_back(tempData);

		circle.pop_back();//remove last point, because it is same as the first one


		std::vector<GenericVertexData> filledCircle;
		for(const GenericVertexData & data : circle)
		{
			filledCircle.push_back(GenericVertexData(data.position(),convertColour(it.value()->getColor(),it.value()->getTransparency())));
		}
		filledCircle.push_back(GenericVertexData(QVector4D(originX, originY, 0.0f, 1.0f), convertColour( it.value()->getColor(),it.value()->getTransparency())));// add center so,circles could be drawn more effectively in opengl
		m_pfilledCircleData.push_back(filledCircle);
	}

}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CUserMapsRenderer::updatePolygons(const QMap<int, QSharedPointer<CUserMapArea> >& loadedArea)
///
/// \brief	Add polygon points
///
/// \param  loadedArea - received areas that should be drawn
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePolygons(const QMap<int, QSharedPointer<CUserMapArea> >& loadedAreas) {

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

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	QMap<int, QSharedPointer<CUserMapCircle> >::Iterator it;

	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );


	for (QMap<int, QSharedPointer<CUserMapArea> >::const_iterator it = loadedAreas.constBegin(); it != loadedAreas.constEnd() ; it++)
	{
		std::vector<GenericVertexData> polygon; //test case polygon ,will be deleted

		for(const CPosition & point : it.value()->getPoints())
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


			polygon.push_back( GenericVertexData(QVector4D( static_cast<float>(xPos), static_cast<float>(yPos), 0.0f, 1.0f), convertColour(it.value()->getOutlineColor())));
		}

		CUserMapsVertexData tempData;
		tempData.setVertexData(polygon);
		setLineStyle(tempData, it.value()->getLineStyle(), it.value()->getLineWidth());
		m_pPolygonData.push_back(tempData);

		std::vector<GenericVertexData>result;
		Triangulate::Process(polygon, result); //triangulate received points

		for(GenericVertexData &data : result)
			data.setColor(convertColour(it.value()->getColor(), it.value()->getTransparency()));

		m_pfilledPolygonData.push_back(result);

	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CUserMapsRenderer::updatePointsData(const QMap<int, QSharedPointer<CUserMapPoint> > &uPointData
///
/// \brief	Add textures that should be drawn
///
/// \param uPointData- textures details
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePointsData(const QMap<int, QSharedPointer<CUserMapPoint> > &uPointData)
{
	//TODO AM: why not just pointData?
	for(const QSharedPointer<CUserMapPoint>& uPoint : uPointData)
	{

		MapPoint data;
		// View Origin
		qreal originX = 0;
		qreal originY = 0;
		CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

		// Geo Origin
		qreal offsetX = 0;
		qreal offsetY = 0;
		CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );

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

		QString strFileName = QString(":/") + data.m_icon + ".png";

		m_pPoints.push_back(data);
		m_pTexture.push_back( QSharedPointer<CImageTexture>(new CImageTexture( strFileName, colour)));
	}
}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawPoints(QOpenGLFunctions *func)
///
/// \brief  Handles mouse move event.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawPoints(QOpenGLFunctions *func) {

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
/// \fn     CUserMapsRenderer::drawLines(QOpenGLFunctions *func)
///
/// \brief  Draws Lines.
///
/// \param  func - Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawLines(QOpenGLFunctions *func) {

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
/// \fn     CUserMapsRenderer::drawPolygons(QOpenGLFunctions *func)
///
/// \brief  Draws polygons.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawPolygons(QOpenGLFunctions *func) {

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
/// \fn     CUserMapsRenderer::drawfilledPolygons(QOpenGLFunctions *func)
///
/// \brief  Draws polygons.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawfilledPolygons(QOpenGLFunctions *func) {

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
/// \fn     CUserMapsRenderer::drawfilledCircles(QOpenGLFunctions *func)
///
/// \brief  Draws Circles.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawfilledCircles(QOpenGLFunctions *func) {


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

	for(uint  i = 0; i<m_pfilledCircleData.size(); i++) {
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
/// \fn     CUserMapsRenderer::drawCircles(QOpenGLFunctions *func)
///
/// \brief  Draws outline Circle.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawCircles(QOpenGLFunctions *func) {

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
/// \fn     CUserMapsRenderer::addPointstoBuffer()
///
/// \brief  add points from vector to buffer
///
/// \param  no params
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::addPointstoBuffer() {
	m_PointBuf = QSharedPointer<CVertexBuffer>( new CVertexBuffer(m_pPointData.data(), m_pPointData.size()));
}



////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer , const std::vector<std::vector<GenericVertexData>> &data)
///
/// \brief  add genericvertexdata to buffer
///
/// \param  buffer - vertex buffer
///         data- points that form a shape
////////////////////////////////////////////////////////////////////////////////
int CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer , const std::vector<std::vector<GenericVertexData>> &data) {
	uint counter = 0;

	for( uint  i = 0; i < data.size(); i++) {
		counter += data[i].size();
	}

	std::vector<GenericVertexData> vertices;
	vertices.reserve(counter);

	for( uint  i=0; i < data.size(); i++) {
		for( uint j=0; j < data[i].size(); j++) {
			vertices.push_back(data[i][j]);
		}
	}
	buffer = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices.data(), counter));

	return counter;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn     int CUserMapsRenderer::drawMultipleElements( QSharedPointer<CVertexBuffer> &buffer, const std::vector<CUserMapsVertexData> &data)
///
/// \brief  add usermapsvertexdata to buffer
///
/// \param  buffer - vertex buffer
///         data- points that form a shape
////////////////////////////////////////////////////////////////////////////////
int CUserMapsRenderer::drawMultipleElements( QSharedPointer<CVertexBuffer> &buffer, const std::vector<CUserMapsVertexData> &data)
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
/// \fn     CUserMapsRenderer::logOpenGLErrors()
///
/// \brief  Opengl logger.
///
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
/// \fn     CUserMapsRenderer::addText( QString text,double x, double y, QVector4D colour,TextAlignment alignment)
///
/// \brief  This function is used for writing text
///
/// \param  text-text that will be written
///        x-x position of text
///        y-y position
///        colour-colour of the text
///        alignment-specifies text alignment
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::addText( QString text,double x, double y, QVector4D colour,TextAlignment alignment)
{
	m_tgtTextRenderer.addText( text, static_cast<int> (x ), static_cast<int> ( y ), FONT_PT_SIZE, colour, alignment );
}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::testCircle(qreal originX, qreal originY)
///
/// \brief  This function is used for measuring drawing speed
///
/// \param  origin x - x-position of the center
///        x-x position of the center
///        y-y position of the center
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::testCircle(qreal originX, qreal originY) {
	double radius = CViewCoordinates::Instance()->getRadiusPixels() / 128;

	int bufferIndex = 0;
	std::vector<GenericVertexData> circle;
	std::vector<GenericVertexData> circle2;
	std::vector<GenericVertexData> circle3;

	// Draw the circle (line strip)
	for ( bufferIndex = 0 ; bufferIndex < rbDegrees; bufferIndex += 8 )//if circle is not round enough use ++ instead of 8
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
/// \fn     CUserMapsRenderer::read(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,QOpenGLFunctions *func)
///
/// \brief  This function should read colour data from pixel
///
/// \param x - x-coordinates of the first pixel
///        y-y coordinates of the first pixel
///        width-width of the pixel rectangle, should be 1
///        height-height of the pixel rectangle, should be 1
///        format-format of the pixel data
///        type- data type of the pixel data
///        func-pointer that points to qopenglfunctions
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::read( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, QOpenGLFunctions *func)
{
	func->glFlush();
	func->glFinish();
	func->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned char data[4] = { 0, 0, 0, 0 };

	func->glReadPixels( x, y, width, height, format, type, data );
	CLoggingLib::logging( E_DEBUGGING )<<endl<<"Colours of<<"<<x<<"and"<<y<<"are r:" <<data[0] << " g" <<data[1]<<" b" <<data[2] <<" a" << data[3];

}


////////////////////////////////////////////////////////////////////////////////
/// \fn     QVector4D CUserMapsRenderer::convertColour(int col, , float opacity)//
/// \brief  This function is used for converting colour into 4d vector
///
/// \param col- colour that should be converted
///        opacity- opacity
///
////////////////////////////////////////////////////////////////////////////////
QVector4D CUserMapsRenderer::convertColour(const QColor &col, float opacity)
{
	//TODO AM: 0 - 1 or 0 - 255 ?
	return QVector4D(col.red(), col.green(), col.blue(), opacity);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::setLineStyle(CUserMapsVertexData& tempData, EUserMapLineStyle lineStyle, float lineWidth)
///
/// \brief  This function is used for setting up linestyles
///
/// \param tempData - data that should be drawn
///        lineStyle- enumerated lineStyle
///        lineWidth-Width of the line
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::setLineStyle(CUserMapsVertexData& tempData, EUserMapLineStyle lineStyle, float lineWidth)
{
	switch (lineStyle) {
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


