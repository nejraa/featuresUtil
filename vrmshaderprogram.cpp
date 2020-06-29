#include "vrmshaderprogram.h"
#include "../OpenGLBaseLib/distancevertexdata.h"

#include <memory>

CVRMShaderProgram::CVRMShaderProgram()
    : CShaderProgram(new QOpenGLShaderProgram)
    , m_shResolutionLoc( nullptr )
    , m_shDashSizeLoc( nullptr )
    , m_shGapSizeLoc( nullptr )
    , m_shMvpMatrixLoc( nullptr )
{
    vrmShaderSetup();

    m_shResolutionLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_resolution"));
    m_shDashSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_dashSize"));
    m_shGapSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_gapSize"));
    m_shMvpMatrixLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "entityMvp"));

    m_shVertexLocation = m_pShaderProgram->attributeLocation("entityPos");
    m_shColLocation = m_pShaderProgram->attributeLocation("entityCol");
    m_shDistanceLocation = m_pShaderProgram->attributeLocation("inDist");

}

CVRMShaderProgram::~CVRMShaderProgram()
{
}

void CVRMShaderProgram::vrmShaderSetup()
{
    // Compile vertex shader
    if (!m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vrmVertexShader.glsl"))
        qDebug() << "m_pShaderProgram->addShaderFromSourceFile QOpenGLShader::Vertex failed!";

    // Compile fragment shader
    if (!m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/vrmFragShader.glsl"))
        qDebug() << "m_pShaderProgram->addShaderFromSourceFile QOpenGLShader::Fragment failed!";

    // Link shader pipeline
    if (!m_pShaderProgram->link())
        qDebug() << "m_pShaderProgram->link() failed!";
}

void CVRMShaderProgram::bind()
{
    m_pShaderProgram->bind();
}

void CVRMShaderProgram::release()
{
    m_pShaderProgram->release();
}

void CVRMShaderProgram::setResolution(float nWidth, float nHeight)
{
    m_shResolutionLoc->setValue(nWidth,nHeight);
}

void CVRMShaderProgram::setDashSize(float nDash)
{
    m_shDashSizeLoc->setValue(nDash);
}

void CVRMShaderProgram::setGapSize(float nGap)
{
    m_shGapSizeLoc->setValue(nGap);
}

void CVRMShaderProgram::setMVPMatrix(QMatrix4x4 mvp)
{
    m_shMvpMatrixLoc->setValue(mvp);
}

void CVRMShaderProgram::setupVertexState()
{
    // Offset for position
    int offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    m_pShaderProgram->enableAttributeArray(m_shVertexLocation);
    m_pShaderProgram->setAttributeBuffer(m_shVertexLocation, GL_FLOAT, offset, 4, sizeof(DistanceVertexData));

    // Offset for position
    offset += sizeof(QVector4D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    m_pShaderProgram->enableAttributeArray(m_shColLocation);
    m_pShaderProgram->setAttributeBuffer(m_shColLocation, GL_FLOAT, offset, 4, sizeof(DistanceVertexData));

    // Offset for Distance
    offset += sizeof(QVector4D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    m_pShaderProgram->enableAttributeArray(m_shDistanceLocation);
    m_pShaderProgram->setAttributeBuffer(m_shDistanceLocation, GL_FLOAT, offset, 1, sizeof(DistanceVertexData));
}

void CVRMShaderProgram::cleanupVertexState()
{
    m_pShaderProgram->disableAttributeArray(m_shVertexLocation);
    m_pShaderProgram->disableAttributeArray(m_shColLocation);
    m_pShaderProgram->disableAttributeArray(m_shDistanceLocation);
}
