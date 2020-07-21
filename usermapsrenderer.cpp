////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsrenderer.h
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

#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69 ///<taken from opengl specifications
#endif
const int CIRCLE_BUFFER_SIZE = 362; ///<points used for drawing circles
const int POINT_BUFFER_SIZE = 100; // will be removed
const bool LOG_OPENGL_ERRORS = false; ///<used for open gl errors
const int rbDegrees = 360; ///< a circle has 360 degrees
static const int FONT_PT_SIZE = 20; ///< font

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::CUserMapsRenderer()
///
/// \brief  Constructor
///
////////////////////////////////////////////////////////////////////////////////
CUserMapsRenderer::CUserMapsRenderer()
	: CBaseRenderer("UserMapsView", OGL_TYPE::PROJ_ORTHO),
	  m_PointBuf(nullptr),
	  m_LineBuf(nullptr),
	  m_CircleBuf(nullptr),
	  m_PolygonBuf(nullptr),
	  m_pOpenGLLogger(nullptr),
	  m_pMapShader(nullptr)

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
/// \fn	CCUserMapsRenderer::render()
///
/// \brief	Called by the Qt framework to render shapes and text.
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
/// \param	item	The CBearingScaleLayer to synchronise with.
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

	float pixelsInMm = static_cast<float>(CViewCoordinates::Instance()->getScreenMmToPixels());
	if ( pixelsInMm == 0.0f )
		return;


	float imgWidthInMM = m_tgTexture[0]->imageWidth() /20.0f;	// Images are designed to be 20 texels/mm
	float textureWidthInPixels = imgWidthInMM * pixelsInMm;	// Total width in pixels
	float imgHeightInMM = m_tgTexture[0]->imageHeight() /20.0f;
	float textureHeightInPixel = imgHeightInMM * pixelsInMm;

	m_tgTexture[0]->setWidth(textureWidthInPixels/2.0f); // set width for one side (left/right)
	m_tgTexture[0]->setHeight(textureHeightInPixel/2.0f);
	m_tgTexture[0]->setProjection(left, right,bottom,top);


	m_pfilledPolygonData.clear();
	m_pCircleData.clear();
	m_pLineData.clear();
	m_pPolygonData.clear();
	updatePoint();
	updatefillPolygon();
	updatefillCircle();
	updateLine();
	updateCircle();
	updatePolygon();
	//updateText();

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
					m_pOpenGLLogger = new QOpenGLDebugLogger();//should be on this );
					bool bOk = m_pOpenGLLogger->initialize();
					if ( bOk )
						qDebug() << "\nCUserMapsRenderer OpenGL debug logging initialized";
					else
						qDebug() << "\nCUserMapsRenderer OpenGL debug logging initialization failed";
				}
			}
		}
	}
	for( int i = 0; i < 1; ++i )
	{
		// Create textures from .png files for each digit
		QString qstrNum = QString(":/HENSOLDT_White.png");
		m_tgTexture.push_back( QSharedPointer<CImageTexture>(new CImageTexture( qstrNum, QVector4D(0.0f,1.0f,0.0f, 1.0f))));
	}


	// Initialise the string renderer
	QOpenGLFunctions* openGL = QOpenGLContext::currentContext()->functions();

	if( openGL->glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE )
	{
		m_stringRenderer.init( framebufferObject(), &m_textureShader );

		// Get the view dimensions
		qreal left = 0;
		qreal right = 0;
		qreal top = 0;
		qreal bottom = 0;
		CViewCoordinates::Instance()->getViewDimensions( left, right, bottom, top );
		m_stringRenderer.setScreenGeometry( QRect( static_cast<int> ( left ),
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

	drawPoint(func);
	drawfilledPolygon(func);
	drawfilledCircle(func);
	drawLine(func);
	drawCircle(func);
	drawPolygon(func);

	read(960.0f,540.0f,1,1, GL_RGBA, GL_UNSIGNED_BYTE,func);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CUserMapsRenderer::renderTextures()
///
/// \brief	Render the textures and text.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::renderTextures()
{

	m_textureShader.bind();

	QMatrix4x4 matrix;

	// Set position
	matrix.translate(460.0f,240.0f, 0.0f );

	float fScaleWidth = m_tgTexture[0]->getWidth();
	float fScaleHeight = m_tgTexture[0]->getHeight();

	matrix.scale(fScaleWidth,fScaleHeight, 0.0f);

	// Bind the target texture
	m_tgTexture[0]->bindTexure();

	// Set the projection matrix
	m_textureShader.setMVPMatrix(m_tgTexture[0]->getProjection() * matrix);

	// Set the texture colour
	m_textureShader.setTexUserColour( QVector4D(1.0f,1.0f,1.0f, 1.0f));

	// Use texture unit 0 for the sampler
	m_textureShader.setTextureSampler(0);

	// Draw the target
	m_tgTexture[0]->drawTexture(&m_textureShader);

	// Release the texture now that we are finished with it
	m_tgTexture[0]->releaseTexture();

	m_stringRenderer.renderText();
	m_textureShader.release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CBearingScaleRenderer::updatePoint()
///
/// \brief	Generate the drawing data to draw the point
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePoint() {

	if( !m_PointBuf.isNull() )
	{
		m_PointBuf.clear();
		m_PointBuf = nullptr;
	}

	GenericVertexData vertices[1];

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	int bufferIndex=0;

	float x = static_cast<float>(originX + 6);
	float y = static_cast<float>(originY + 6);

	vertices[bufferIndex].setPosition(QVector4D(1000.0f,2.0f, 0.0f, 1.0f));
	vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));
	m_pPointData.push_back(vertices[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CBearingScaleRenderer::updateLine()
///
/// \brief	Add line points so line could be drawn
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateLine() {
	if( !m_LineBuf.isNull() )
	{
		m_LineBuf.clear();
		m_LineBuf = nullptr;
	}

	std::vector<GenericVertexData> line;//test data,will be deleted
	std::vector<GenericVertexData> line2;//test data,will be deleted
	std::vector<GenericVertexData> line3;//test data,will be deleted

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );

	float x = static_cast<float>(originX);
	float y = static_cast<float>(originY); //instances should be taken from mapsmanager singleton

	line.push_back( GenericVertexData(QVector4D( x, y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	line.push_back( GenericVertexData(QVector4D( x+1000,38.0, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));

	line.push_back( GenericVertexData(QVector4D( 65535,65535 ,65535 , 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));//add end of the line

	CUserMapsVertexData tempData;
	tempData.setVertexData(line);
	tempData.setDashSize(30.0f);
	tempData.setGapSize(15.0f);
	tempData.setDotSize(10.0f);

	m_pLineData.push_back(tempData);

	line2.push_back( GenericVertexData(QVector4D( x+600, y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	line2.push_back( GenericVertexData(QVector4D( x+600, y+200, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	line2.push_back( GenericVertexData(QVector4D( 65535,65535 ,65535 , 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));//add end of the line
	//addText("text",x+600,y,QVector4D(0.0f,1.0f,0.0f, 1.0f),TextAlignment::CENTRE);

	qDebug()<<"Line 2 x is "<<x+400<<"and y is "<<y;

	CUserMapsVertexData tempData2;
	tempData2.setVertexData(line2);
	tempData2.setDashSize(30.0f);
	tempData2.setGapSize(0.0f);
	tempData2.setDotSize(0.0f);
	m_pLineData.push_back(tempData2);

	line3.push_back( GenericVertexData(QVector4D( x+800, y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	line3.push_back( GenericVertexData(QVector4D( x+800, y+200, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));


	CUserMapsVertexData tempData3;
	tempData3.setVertexData(line3);
	tempData3.setDashSize(30.0f);
	tempData3.setGapSize(0.0f);
	tempData3.setDotSize(0.0f);
	m_pLineData.push_back(tempData3);


	addText("text",x+600,y-200,QVector4D(0.0f,1.0f,0.0f, 1.0f),TextAlignment::CENTRE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CBearingScaleRenderer::updateCircle()
///
/// \brief	Add Circle points so circle could be drawn.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateCircle() {
	if( !m_CircleBuf.isNull() )
	{
		m_CircleBuf.clear();
		m_CircleBuf = nullptr;
	}

	std::vector<GenericVertexData> circle;

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
	double radius = CViewCoordinates::Instance()->getRadiusPixels()/2;

	int bufferIndex = 0;

	// Draw the circle (line strip)
	for ( bufferIndex=0; bufferIndex<=rbDegrees; bufferIndex+=8 )
	{
		double angle = 2 * M_PI * bufferIndex / rbDegrees;
		float x = static_cast<float>(20+originX + (radius * std::sin(angle)));
		float y = static_cast<float>(originY - (radius * std::cos(angle)));

		// Set Vertex data
		circle.push_back( GenericVertexData(QVector4D( x, y, 0.0f, 1.0f),QVector4D(0.0f,0.0f,1.0f, 1.0f)));

		// Check if it is the last point
		if( bufferIndex == rbDegrees )
		{
			// Change to transparent colour at the same position
			bufferIndex++;
			circle.push_back( GenericVertexData(QVector4D( x, y, 0.0f, 1.0f),QVector4D(0.0f,0.0f,1.0f, 1.0f)));

		}
	}
	CUserMapsVertexData tempData;
	tempData.setVertexData(circle);
	tempData.setDashSize(30.0f);
	tempData.setGapSize(15.0f);
	tempData.setDotSize(10.0f);

	m_pCircleData.push_back(tempData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CUserMapsRenderer::updatefillCircle()
///
/// \brief	fill circle
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatefillCircle() {
	if( !m_CircleBuf.isNull() )
	{
		m_CircleBuf.clear();
		m_CircleBuf = nullptr;
	}
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );


	GenericVertexData vertices[CIRCLE_BUFFER_SIZE];
	std::vector<GenericVertexData> circle;
	std::vector<GenericVertexData> circle2; //test data,will be deleted
	std::vector<GenericVertexData> circle3;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );

	double radius = CViewCoordinates::Instance()->getRadiusPixels()/2;

	int bufferIndex = 0;

	// Draw the circle (line strip)
	for ( bufferIndex=0; bufferIndex<=rbDegrees; bufferIndex+=8 )//if circle is not round enough use ++ instead of 8
	{
		double angle = 2 * M_PI * bufferIndex / rbDegrees;
		float x = static_cast<float>(originX + (radius * std::sin(angle)));
		float y = static_cast<float>(originY - (radius * std::cos(angle)));

		// Set Vertex data
		circle.push_back(GenericVertexData(QVector4D(x, y, 0.0f, 1.0f),QVector4D(1.0f,0.0f,0.0f, 0.5f)));

		// Check if it is the last point
	}
	if( bufferIndex == rbDegrees )
	{
		//add center so triangle fan could be used for drawing

		bufferIndex++;
		circle.push_back(GenericVertexData(QVector4D(originX, originY, 0.0f, 1.0f),QVector4D(1.0f,0.0f,0.0f, 0.5f)));
	}

	bufferIndex=0;


	// Draw 2nd circle (line strip)
	for ( bufferIndex=0; bufferIndex<=rbDegrees; bufferIndex+=8 )//if circle is not round enough use ++ instead of 8
	{
		double angle = 2 * M_PI * bufferIndex / rbDegrees;
		float x = static_cast<float>(20+originX + (radius * std::sin(angle)));
		float y = static_cast<float>(originY - (radius * std::cos(angle)));

		x=x-600;

		// Set Vertex data
		circle2.push_back(GenericVertexData(QVector4D(x, y, 0.0f, 1.0f),QVector4D(1.0f,0.0f,0.0f, 0.5f)));

		// Check if it is the last point
		if( bufferIndex == rbDegrees )
		{
			//add center so triangle fan could be used for drawing

			bufferIndex++;
			circle2.push_back(GenericVertexData(QVector4D(originX, originY, 0.0f, 1.0f),QVector4D(1.0f,0.0f,0.0f, 0.5f)));
		}
	}
	m_pfilledCircleData.push_back(circle);
	m_pfilledCircleData.push_back(circle2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CBearingScaleRenderer::updatePolygon()
///
/// \brief	Add polygon points
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePolygon() {
	if( !m_PolygonBuf.isNull() )
	{
		m_PolygonBuf.clear();
		m_PolygonBuf = nullptr;
	}

	std::vector<GenericVertexData> polygon; //test case polygon ,will be deleted

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );

	float x = static_cast<float>(originX + 400);
	float y = static_cast<float>(originY); //doraditi da uzima stavke iz singletona mapsmanager-a kad bude gotov

	polygon.push_back( GenericVertexData(QVector4D( x, y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	polygon.push_back( GenericVertexData(QVector4D(x+100,y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	polygon.push_back( GenericVertexData(QVector4D(x+100,y+100, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	polygon.push_back( GenericVertexData(QVector4D( x, y+100, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	polygon.push_back(polygon[0]);
	polygon.push_back( GenericVertexData(QVector4D( 65535,65535 ,65535 , 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));

	CUserMapsVertexData tempData;
	tempData.setVertexData(polygon);
	tempData.setDashSize(30.0f);
	tempData.setGapSize(15.0f);
	tempData.setDotSize(10.0f);

	m_pPolygonData.push_back(tempData);

}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	CBearingScaleRenderer::updatefillPolygon()
///
/// \brief	fill polygon
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatefillPolygon() {
	if( !m_PolygonBuf.isNull() )
	{
		m_PolygonBuf.clear();
		m_PolygonBuf = nullptr;
	}

	// Get coordiante system data
	qreal originX = 0.0;
	qreal originY = 0.0;
	CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
	qreal offsetX = 0.0;
	qreal offsetY = 0.0;
	CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );

	float x = static_cast<float>(originX + 400);
	float y = static_cast<float>(originY); //should take data from singleton

	std::vector<GenericVertexData>vertices; // test data

	vertices.push_back(GenericVertexData(QVector4D( x, y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices.push_back(GenericVertexData(QVector4D( x+100,y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices.push_back(GenericVertexData(QVector4D( x+100, y+100, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices.push_back(GenericVertexData(QVector4D( x, y+100, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));

	std::vector<GenericVertexData>result;
	Triangulate::Process(vertices,result);

	std::vector<GenericVertexData>vertices2;
	std::vector<GenericVertexData>result2;

	y=y-500;

	vertices2.push_back(GenericVertexData(QVector4D( x, y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices2.push_back(GenericVertexData(QVector4D( x+100,y, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices2.push_back(GenericVertexData(QVector4D( x+100, y+100, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices2.push_back(GenericVertexData(QVector4D( x+50, y+150, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));
	vertices2.push_back(GenericVertexData(QVector4D( x, y+100, 0.0f, 1.0f),QVector4D(0.0f,1.0f,0.0f, 1.0f)));

	Triangulate::Process(vertices2,result2);

	m_pfilledPolygonData.push_back(result);
	m_pfilledPolygonData.push_back(result2);

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawPoint(QOpenGLFunctions *func)
///
/// \brief  Handles mouse move event.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawPoint(QOpenGLFunctions *func) {

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

	m_PointBuf=QSharedPointer<CVertexBuffer>(new CVertexBuffer(m_pPointData.data(),m_pPointData.size()));

	m_PointBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawPoint() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_primShader.setupVertexState();

	func->glDrawArrays(GL_POINTS, 0, m_pPointData.size());
	// Tidy up
	m_primShader.cleanupVertexState();

	m_PointBuf->release();

	m_primShader.release();

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawLine(QOpenGLFunctions *func)
///
/// \brief  Draws Line.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawLine(QOpenGLFunctions *func) {

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

	m_pMapShader->setDashSize(30.0f);
	m_pMapShader->setGapSize(15.0f);
	m_pMapShader->setDotSize(10.0f);

	int counter=drawMultipleElements(m_LineBuf,m_pLineData);

	m_LineBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CUserMapsRenderer::drawLine() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_pMapShader->setupVertexState();
	m_pMapShader->setDashSize(30.0f);
	m_pMapShader->setGapSize(0.0f);
	m_pMapShader->setDotSize(0.0f);

	func->glLineWidth(100);
	func->glDrawArrays(GL_LINE_STRIP,0 ,counter);

	// Tidy up
	m_pMapShader->cleanupVertexState();

	m_LineBuf->release();

	m_pMapShader->release();

}


////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawPolygon(QOpenGLFunctions *func)
///
/// \brief  Draws polygon.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawPolygon(QOpenGLFunctions *func) {

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

	m_pMapShader->setDashSize(20.0f);
	m_pMapShader->setGapSize(10.0f);
	m_pMapShader->setDotSize(15.0f);

	int counter=drawMultipleElements(m_PolygonBuf,m_pPolygonData);

	m_PolygonBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "COwnshipRenderer::drawPolygon() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_pMapShader->setupVertexState();

	func->glDrawArrays(GL_LINE_STRIP, 0, counter);

	// Tidy up
	m_pMapShader->cleanupVertexState();

	m_PolygonBuf->release();

	m_pMapShader->release();

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawfilledPolygon(QOpenGLFunctions *func)
///
/// \brief  Draws polygon.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawfilledPolygon(QOpenGLFunctions *func) {

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

	int counter=drawMultipleElements(m_filledPolygonBuf,m_pfilledPolygonData);
	m_filledPolygonBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "COwnshipRenderer::drawPolygon() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_primShader.setupVertexState();

	//draw
	func->glDrawArrays(GL_TRIANGLES, 0, counter);

	// Tidy up
	m_primShader.cleanupVertexState();

	m_filledPolygonBuf->release();

	m_primShader.release();

}

////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawfilledCircle(QOpenGLFunctions *func)
///
/// \brief  Draws Circle.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawfilledCircle(QOpenGLFunctions *func) {


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
	int counter= drawMultipleElements(m_InlineCircleBuf,m_pfilledCircleData);
	m_InlineCircleBuf->bind();


	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CBearingScaleRenderer::drawScale() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_primShader.setupVertexState();
	int offset=0; //offset

	//Draw inline and outline circle from data in the VBOs


	for(std::vector<std::vector<GenericVertexData>>::size_type i=0;i<m_pfilledCircleData.size();i++) {
		func->glDrawArrays(GL_TRIANGLE_FAN, offset, m_pfilledCircleData[i].size());
		offset=offset + m_pfilledCircleData[i].size();
	}
	func->glLineWidth( 1 );


	// Tidy up
	m_InlineCircleBuf->release();

	m_primShader.cleanupVertexState();


	m_primShader.release();

}


////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawCircle(QOpenGLFunctions *func)
///
/// \brief  Draws outline Circle.
///
/// \param  Pointer that points to QOpenGLFunctions.
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::drawCircle(QOpenGLFunctions *func) {


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
	m_pMapShader->setDashSize(30.0f);
	m_pMapShader->setGapSize(4.0f);
	m_pMapShader->setDotSize(1.0f);

	// Tell OpenGL which VBOs to use
	int counter= drawMultipleElements(m_CircleBuf,m_pCircleData);
	m_CircleBuf->bind();

	// Check Frame buffer is OK
	GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( e != GL_FRAMEBUFFER_COMPLETE)
		qDebug() << "CBearingScaleRenderer::drawScale() failed! Not GL_FRAMEBUFFER_COMPLETE";

	m_pMapShader->setupVertexState();
	func->glLineWidth(1);

	func->glDrawArrays(GL_LINE_STRIP, 0,counter);


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
	GenericVertexData vertices[m_pPointData.size()];
	for(std::vector<GenericVertexData>::size_type  i=0;i<m_pPointData.size();i++) {
		vertices[i] =m_pPointData.at(i);
	}
	m_PointBuf=QSharedPointer<CVertexBuffer>(new CVertexBuffer(vertices,m_pPointData.size()));
}



////////////////////////////////////////////////////////////////////////////////
/// \fn     CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer , const std::vector<std::vector<GenericVertexData>> &data)
///
/// \brief  add data to buffer
///
/// \param  buffer - vertex buffer
///         data- points that form a shape
////////////////////////////////////////////////////////////////////////////////
int CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer , const std::vector<std::vector<GenericVertexData>> &data) {
	uint counter=0;

	for(uint  i=0;i<data.size();i++) {
		counter+=data[i].size();
	}

	std::vector<GenericVertexData> vertices;
	vertices.reserve(counter);

	for(uint  i=0;i<data.size();i++) {
		for(uint j=0;j<data[i].size();j++) {
			vertices.push_back(data[i][j]);
		}
	}
	buffer=QSharedPointer<CVertexBuffer>(new CVertexBuffer(vertices.data(),counter));

	return counter;
}

int CUserMapsRenderer::drawMultipleElements(QSharedPointer<CVertexBuffer> &buffer, const std::vector<CUserMapsVertexData> &data)
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

	buffer = QSharedPointer<CVertexBuffer>(new CVertexBuffer(vertices.data(), totalSize));

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
	m_stringRenderer.addText( text, static_cast<int> (x ), static_cast<int> ( y ),FONT_PT_SIZE,colour, alignment );
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
	double radius = CViewCoordinates::Instance()->getRadiusPixels()/128;

	int bufferIndex = 0;
	std::vector<GenericVertexData> circle;
	std::vector<GenericVertexData> circle2;
	std::vector<GenericVertexData> circle3;

	// Draw the circle (line strip)
	for ( bufferIndex=0; bufferIndex<rbDegrees; bufferIndex+=8 )//if circle is not round enough use ++ instead of 8
	{
		double angle = 2 * M_PI * bufferIndex / rbDegrees;
		float x = static_cast<float>(originX + (radius * std::sin(angle)));
		float y = static_cast<float>(originY - (radius * std::cos(angle)));

		// Set Vertex data
		circle.push_back(GenericVertexData(QVector4D(x, y, 0.0f, 1.0f),QVector4D(1.0f,0.0f,0.0f, 1.0f)));

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
///        width-width of the pixel rectangle
///        height-height of the pixel rectangle
///        format-format of the pixel data
///        type- data type of the pixel data
///        func-pointer that points to qopenglfunctions
////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::read(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,QOpenGLFunctions *func) {
	func->glFlush();
	func->glFinish();
	func->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned char data[4] = {0,0,0,0};

	func->glReadPixels(x,y, 1,1,format,type, data);//TO DO:: figure out why line colours are not correctly picked

	qDebug()<<"Colours are r:"<<data[0]<<" g"<<data[1]<<" b"<<data[2]<<" a"<<data[3];
}



