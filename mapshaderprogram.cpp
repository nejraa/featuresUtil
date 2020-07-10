#include "mapshaderprogram.h"
#include "../OpenGLBaseLib/genericvertexdata.h"

CMapShaderProgram::CMapShaderProgram()
	: CShaderProgram (new QOpenGLShaderProgram())
	, m_shResolutionLoc( nullptr )
	, m_shDashSizeLoc( nullptr )
	, m_shGapSizeLoc( nullptr )
	,m_shDotSizeLoc( nullptr )
	, m_shMvpMatrixLoc( nullptr )
{
	mapShaderSetup();

	m_shResolutionLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_resolution"));
	m_shDashSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_dashSize"));
	m_shGapSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_gapSize"));
	m_shDotSizeLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "u_dotSize"));
	m_shMvpMatrixLoc = QSharedPointer<CShaderProgramUniform>(new CShaderProgramUniform(CShaderProgram(m_pShaderProgram), "entityMvp"));
	m_shVertexLocation = m_pShaderProgram->attributeLocation("entityPos");
	m_shColLocation = m_pShaderProgram->attributeLocation("entityCol");
}

CMapShaderProgram::~CMapShaderProgram()
{
}

void CMapShaderProgram::mapShaderSetup( )
{
	// Compile vertex shader
	if (!m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/mapsVertexShader.glsl"))
	{
		qDebug() << m_pShaderProgram->log();
		qDebug() << "m_pShaderProgram->addShaderFromSourceFile QOpenGLShader::Vertex failed!";
	}

	// Compile fragment shader
	if (!m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/mapsFragShader.glsl"))
		qDebug() << "m_pShaderProgram->addShaderFromSourceFile QOpenGLShader::Fragment failed!";

	// Link shader pipeline
	if (!m_pShaderProgram->link())
		qDebug() << "m_pShaderProgram->link() failed!";
}

void CMapShaderProgram::bind()
{
	m_pShaderProgram->bind();
}

void CMapShaderProgram::release()
{
	m_pShaderProgram->release();
}

void CMapShaderProgram::setResolution(float nWidth, float nHeight)
{
	m_shResolutionLoc->setValue(nWidth,nHeight);
}

void CMapShaderProgram::setDashSize(float nDash)
{
	m_shDashSizeLoc->setValue(nDash);
}

void CMapShaderProgram::setGapSize(float nGap)
{
	m_shGapSizeLoc->setValue(nGap);
}

void CMapShaderProgram::setDotSize(float nDot)
{
	m_shDotSizeLoc->setValue(nDot);
}

void CMapShaderProgram::setMVPMatrix(QMatrix4x4 mvp)
{
	m_shMvpMatrixLoc->setValue(mvp);
}

void CMapShaderProgram::setupVertexState()
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

void CMapShaderProgram::cleanupVertexState()
{
	m_pShaderProgram->disableAttributeArray(m_shVertexLocation);
	m_pShaderProgram->disableAttributeArray(m_shColLocation);
}

