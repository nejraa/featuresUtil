#include "eblshaderprogram.h"
#include "../OpenGLBaseLib/genericvertexdata.h"

CEBLShaderProgram::CEBLShaderProgram()
    : CShaderProgram (new QOpenGLShaderProgram())
    , m_shResolutionLoc( nullptr )
    , m_shDashSizeLoc( nullptr )
    , m_shGapSizeLoc( nullptr )
    , m_shMvpMatrixLoc( nullptr )
{
    eblShaderSetup();

    m_shResolutionLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_resolution"));
    m_shDashSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_dashSize"));
    m_shGapSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_gapSize"));
    m_shMvpMatrixLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "entityMvp"));

    m_shVertexLocation = m_pShaderProgram->attributeLocation("entityPos");
    m_shColLocation = m_pShaderProgram->attributeLocation("entityCol");
}

CEBLShaderProgram::~CEBLShaderProgram()
{
}

void CEBLShaderProgram::eblShaderSetup( )
{
    // Compile vertex shader
    if (!m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/NavShaders/eblVertexShader.glsl"))
    {
        qDebug() << m_pShaderProgram->log();
        qDebug() << "m_pShaderProgram->addShaderFromSourceFile QOpenGLShader::Vertex failed!";
    }

    // Compile fragment shader
    if (!m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/NavShaders/eblFragShader.glsl"))
        qDebug() << "m_pShaderProgram->addShaderFromSourceFile QOpenGLShader::Fragment failed!";

    // Link shader pipeline
    if (!m_pShaderProgram->link())
        qDebug() << "m_pShaderProgram->link() failed!";
}

void CEBLShaderProgram::bind()
{
    m_pShaderProgram->bind();
}

void CEBLShaderProgram::release()
{
    m_pShaderProgram->release();
}

void CEBLShaderProgram::setResolution(float nWidth, float nHeight)
{
    m_shResolutionLoc->setValue(nWidth,nHeight);
}

void CEBLShaderProgram::setDashSize(float nDash)
{
    m_shDashSizeLoc->setValue(nDash);
}

void CEBLShaderProgram::setGapSize(float nGap)
{
    m_shGapSizeLoc->setValue(nGap);
}

void CEBLShaderProgram::setMVPMatrix(QMatrix4x4 mvp)
{
    m_shMvpMatrixLoc->setValue(mvp);
}

void CEBLShaderProgram::setupVertexState()
{
    // Offset for position
    int offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    m_pShaderProgram->enableAttributeArray(m_shVertexLocation);
    m_pShaderProgram->setAttributeBuffer(m_shVertexLocation, GL_FLOAT, offset, 4, sizeof(GenericVertexData));

    // Offset for position
    offset += sizeof(QVector4D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    m_pShaderProgram->enableAttributeArray(m_shColLocation);
    m_pShaderProgram->setAttributeBuffer(m_shColLocation, GL_FLOAT, offset, 4, sizeof(GenericVertexData));
}

void CEBLShaderProgram::cleanupVertexState()
{
    m_pShaderProgram->disableAttributeArray(m_shVertexLocation);
    m_pShaderProgram->disableAttributeArray(m_shColLocation);
}

