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
#include <QOpenGLFramebufferObject>
#include "../OpenGLBaseLib/genericvertexdata.h"

const int CIRCLE_BUFFER_SIZE = 362;
const int POINT_BUFFER_SIZE = 100; // will be removed
const bool LOG_OPENGL_ERRORS = false;
const int rbDegrees = 360;

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

CUserMapsRenderer::~CUserMapsRenderer()
{

}


void CUserMapsRenderer::render()
{
    framebufferObject()->bind();
    QOpenGLFunctions* pFunctions = QOpenGLContext::currentContext()->functions();

    // Clear the FBO to transparent black
    pFunctions->glClearColor( 0, 0, 0, 0 );
    pFunctions->glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    // Enable alpha blending
    pFunctions->glEnable (GL_BLEND );
    pFunctions->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	// Set The Blending Function For Translucency

    renderPrimitives( pFunctions );
    renderTextures();

    // Disable blending after use
    pFunctions->glDisable( GL_BLEND );

    framebufferObject()->release();

}

void CUserMapsRenderer::initShader()
{
    if( m_pMapShader == nullptr )
        m_pMapShader = QSharedPointer<CMapShaderProgram>(new CMapShaderProgram());

}


void CUserMapsRenderer::synchronize(QQuickFramebufferObject *item)
{
    Q_UNUSED(item);


    updatefillPolygon();
    //updatefillCircle();
    updateLine();
    updateCircle();
    updatePolygon();

}

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

    m_bGLinit = true;

}

void CUserMapsRenderer::renderPrimitives(QOpenGLFunctions *func)
{


    drawfilledPolygon(func);
  // drawfilledCircle(func);
    drawLine(func);
    drawCircle(func);
    drawPolygon(func);

}

void CUserMapsRenderer::renderTextures()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	CBearingScaleRenderer::updatePoint()
///
/// @brief	Generate the drawing data to draw the point.
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
    float y = static_cast<float>(originY + 6); //doraditi da uzima stavke iz singletona mapsmanager-a kad bude gotov

    vertices[bufferIndex].setPosition(QVector4D(x, y, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(m_PointColour);

    m_PointBuf = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices, 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	CBearingScaleRenderer::updateLine()
///
/// @brief	Generate the drawing data to draw the line.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateLine() {
    if( !m_LineBuf.isNull() )
    {
        m_LineBuf.clear();
        m_LineBuf = nullptr;
    }

    GenericVertexData vertices[2]; //izmijeniti

    // Get coordiante system data
    qreal originX = 0.0;
    qreal originY = 0.0;
    CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
    qreal offsetX = 0.0;
    qreal offsetY = 0.0;
    CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
    int bufferIndex=0;

    float x = static_cast<float>(originX + 10);
    float y = static_cast<float>(originY + 10); //doraditi da uzima stavke iz singletona mapsmanager-a kad bude gotov

    vertices[bufferIndex].setPosition(QVector4D(x,y, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));

    bufferIndex++;

    vertices[bufferIndex].setPosition(QVector4D(x+1000, 38.0f, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));
    bufferIndex++;

    m_LineBuf  = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices, bufferIndex));

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	CBearingScaleRenderer::updateCircle()
///
/// @brief	Generate the drawing data to draw the circle.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updateCircle() {
    if( !m_CircleBuf.isNull() )
    {
        m_CircleBuf.clear();
        m_CircleBuf = nullptr;
    }

    GenericVertexData vertices[CIRCLE_BUFFER_SIZE];

    // Get coordiante system data
    qreal originX = 0.0;
    qreal originY = 0.0;
    CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
    qreal offsetX = 0.0;
    qreal offsetY = 0.0;
    CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
    double radius = CViewCoordinates::Instance()->getRadiusPixels();

    int bufferIndex = 0;
    int k=0;

    // Draw the circle (line strip)
    for ( bufferIndex=0; bufferIndex<=rbDegrees; bufferIndex+=8 )
    {
        double angle = 2 * M_PI * bufferIndex / rbDegrees;
        float x = static_cast<float>(20+originX + (radius * std::sin(angle)));
        float y = static_cast<float>(originY - (radius * std::cos(angle)));

        // Set Vertex data
        vertices[k].setPosition(QVector4D(x, y, 0.0f, 1.0f));
        vertices[k].setColor(QVector4D(0.0f,0.0f,1.0f, 1.0f));

        // Check if it is the last point
        if( bufferIndex == rbDegrees )
        {
            // Change to transparent colour at the same position
            bufferIndex++;
            k++;
            vertices[k].setPosition(QVector4D(x, y, 0.0f, 1.0f));
            vertices[k].setColor(QVector4D(0.0f,0.0f,1.0f,0.0f));
        }
        k++;
    }
     m_CircleBuf = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices, CIRCLE_BUFFER_SIZE));

}

void CUserMapsRenderer::updatefillCircle() {
    if( !m_CircleBuf.isNull() )
      {
          m_CircleBuf.clear();
          m_CircleBuf = nullptr;
      }

      GenericVertexData vertices[CIRCLE_BUFFER_SIZE];

      // Get coordiante system data
      qreal originX = 0.0;
      qreal originY = 0.0;
      CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
      qreal offsetX = 0.0;
      qreal offsetY = 0.0;
      CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
      double radius = CViewCoordinates::Instance()->getRadiusPixels();

      int bufferIndex = 0;

      // Draw the circle (line strip)
      for ( bufferIndex=0; bufferIndex<=rbDegrees; bufferIndex++ )
      {
          double angle = 2 * M_PI * bufferIndex / rbDegrees;
          float x = static_cast<float>(20+originX + (radius * std::sin(angle)));
          float y = static_cast<float>(originY - (radius * std::cos(angle)));

          // Set Vertex data
          vertices[bufferIndex].setPosition(QVector4D(x, y, 0.0f, 1.0f));
          vertices[bufferIndex].setColor(QVector4D(1.0f,0.0f,0.0f, 0.5f));

          // Check if it is the last point
          if( bufferIndex == rbDegrees )
          {
              // Change to transparent colour at the same position

              bufferIndex++;
              vertices[bufferIndex].setPosition(QVector4D(originX, originY, 0.0f, 1.0f));
              vertices[bufferIndex].setColor(QVector4D(1.0f,0.0f,0.0f, 0.5f));
          }
      }
       m_InlineCircleBuf = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices, CIRCLE_BUFFER_SIZE));
       //colour fix required
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	CBearingScaleRenderer::updatePolygon()
///
/// @brief	Generate the drawing data to draw a contour of the polygon.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatePolygon() {
    if( !m_PolygonBuf.isNull() )
    {
        m_PolygonBuf.clear();
        m_PolygonBuf = nullptr;
    }

    GenericVertexData vertices[4]; //izmijeniti

    // Get coordiante system data
    qreal originX = 0.0;
    qreal originY = 0.0;
    CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
    qreal offsetX = 0.0;
    qreal offsetY = 0.0;
    CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
    int bufferIndex=0;

    float x = static_cast<float>(originX + 400);
    float y = static_cast<float>(originY); //doraditi da uzima stavke iz singletona mapsmanager-a kad bude gotov

    vertices[bufferIndex].setPosition(QVector4D( x, y, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));

    bufferIndex++;

    vertices[bufferIndex].setPosition(QVector4D(x+100,y, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));


    bufferIndex++;
    vertices[bufferIndex].setPosition(QVector4D(x+100, y+100, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));


    bufferIndex++;

    vertices[bufferIndex].setPosition(QVector4D(x, y+100, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));
    bufferIndex++;

    m_PolygonBuf  = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices, bufferIndex));

}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	CBearingScaleRenderer::updatefillPolygon()
///
/// @brief	Generate the drawing data to draw the polygon.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CUserMapsRenderer::updatefillPolygon() {
    if( !m_PolygonBuf.isNull() )
    {
        m_PolygonBuf.clear();
        m_PolygonBuf = nullptr;
    }

    GenericVertexData vertices[4]; //izmijeniti

    // Get coordiante system data
    qreal originX = 0.0;
    qreal originY = 0.0;
    CViewCoordinates::Instance()->getViewOriginPixel( originX, originY );
    qreal offsetX = 0.0;
    qreal offsetY = 0.0;
    CViewCoordinates::Instance()->getGeoOriginOffsetPixel( offsetX, offsetY );
    int bufferIndex=0;

    float x = static_cast<float>(originX + 400);
    float y = static_cast<float>(originY); //doraditi da uzima stavke iz singletona mapsmanager-a kad bude gotov

    vertices[bufferIndex].setPosition(QVector4D( x, y, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));

    bufferIndex++;

    vertices[bufferIndex].setPosition(QVector4D(x+100,y, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));


    bufferIndex++;
    vertices[bufferIndex].setPosition(QVector4D(x+100, y+100, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));


    bufferIndex++;

    vertices[bufferIndex].setPosition(QVector4D(x, y+100, 0.0f, 1.0f));
    vertices[bufferIndex].setColor(QVector4D(0.0f,1.0f,0.0f, 1.0f));
    bufferIndex++;

    for(int i=0;i<bufferIndex;i++) {
        sortedelements.push_back(vertices[i]);
    }
    std::sort(sortedelements.begin(),sortedelements.end(), [ ]( const GenericVertexData& el1, const GenericVertexData& el2)
    {
       return el1.position().y() > el2.position().y();
    });

    std::vector<GenericVertexData>result;
    Triangulate::Process(sortedelements,result);

    GenericVertexData vertices2[result.size()];

    for(std::vector<GenericVertexData>::size_type i=0;i<result.size();i++) {
        vertices2[i]=result.at(i);
    }

    m_filledPolygonBuf  = QSharedPointer<CVertexBuffer>( new CVertexBuffer(vertices2, result.size()));

}

////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::drawPoint(QOpenGLFunctions *func)
///
/// brief  Handles mouse move event.
///
/// param  Pointer that points to QOpenGLFunctions.
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

    m_PointBuf->bind();

    // Check Frame buffer is OK
    GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( e != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "CUserMapsRenderer::drawPoint() failed! Not GL_FRAMEBUFFER_COMPLETE";

    m_primShader.setupVertexState();

   //ovdje crtati tacke ispitati kako se dobavljaju

    // Tidy up
    m_primShader.cleanupVertexState();

    m_PointBuf->release();

    m_primShader.release();

}

////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::drawLine(QOpenGLFunctions *func)
///
/// brief  Draws Line.
///
/// param  Pointer that points to QOpenGLFunctions.
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
    m_pMapShader->setGapSize(30.0f);
    m_pMapShader->setDotSize(15.0f);
   // m_pMapShader->setDotSize(5.0f);

    m_LineBuf->bind();

    // Check Frame buffer is OK
    GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( e != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "CUserMapsRenderer::drawLine() failed! Not GL_FRAMEBUFFER_COMPLETE";

    m_pMapShader->setupVertexState();

   //ovdje crtati tacke ispitati kako se dobavljaju
    func->glDrawArrays(GL_LINES, 0, 2);

    // Tidy up
    m_pMapShader->cleanupVertexState();

    m_LineBuf->release();

   m_pMapShader->release();

}


////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::drawPolygon(QOpenGLFunctions *func)
///
/// brief  Draws polygon.
///
/// param  Pointer that points to QOpenGLFunctions.
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
   // m_pMapShader->setDotSize(5.0f);
    m_PolygonBuf->bind();

    // Check Frame buffer is OK
    GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( e != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "COwnshipRenderer::drawPolygon() failed! Not GL_FRAMEBUFFER_COMPLETE";

    m_pMapShader->setupVertexState();

   //ovdje crtati tacke ispitati kako se dobavljaju
    func->glDrawArrays(GL_LINE_LOOP, 0, 4);

    // Tidy up
    m_pMapShader->cleanupVertexState();

    m_PolygonBuf->release();

    m_pMapShader->release();

}

////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::drawfilledPolygon(QOpenGLFunctions *func)
///
/// brief  Draws polygon.
///
/// param  Pointer that points to QOpenGLFunctions.
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

    m_filledPolygonBuf->bind();

    // Check Frame buffer is OK
    GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( e != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "COwnshipRenderer::drawPolygon() failed! Not GL_FRAMEBUFFER_COMPLETE";

    m_primShader.setupVertexState();

   //ovdje crtati tacke ispitati kako se dobavljaju
    func->glDrawArrays(GL_TRIANGLES, 0, 6);

    // Tidy up
    m_primShader.cleanupVertexState();

    m_filledPolygonBuf->release();

    m_primShader.release();

}

////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::drawfilledCircle(QOpenGLFunctions *func)
///
/// brief  Draws Circle.
///
/// param  Pointer that points to QOpenGLFunctions.
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
    m_InlineCircleBuf->bind();

    // Check Frame buffer is OK
    GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( e != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "CBearingScaleRenderer::drawScale() failed! Not GL_FRAMEBUFFER_COMPLETE";

    m_primShader.setupVertexState();

    // Draw inline and outline circle from data in the VBOs
    func->glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_BUFFER_SIZE);
    func->glLineWidth( 1 );


    // Tidy up
    m_InlineCircleBuf->release();

    m_primShader.cleanupVertexState();



    m_primShader.release();

}


////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::drawCircle(QOpenGLFunctions *func)
///
/// brief  Draws outline Circle.
///
/// param  Pointer that points to QOpenGLFunctions.
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
    m_pMapShader->setGapSize(10.0f);
    m_pMapShader->setDotSize(1.0f);

    // Tell OpenGL which VBOs to use
    m_CircleBuf->bind();

    // Check Frame buffer is OK
    GLenum e = func->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( e != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "CBearingScaleRenderer::drawScale() failed! Not GL_FRAMEBUFFER_COMPLETE";

     m_pMapShader->setupVertexState();
    func->glLineWidth(1);

    func->glDrawArrays(GL_LINE_STRIP, 0, CIRCLE_BUFFER_SIZE);


    // Tidy up
     m_pMapShader->cleanupVertexState();

    m_CircleBuf->release();

     m_pMapShader->release();

}



////////////////////////////////////////////////////////////////////////////////
/// fn     CUserMapsRenderer::logOpenGLErrors()
///
/// brief  Opengl logger.
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
