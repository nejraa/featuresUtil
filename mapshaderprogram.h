////////////////////////////////////////////////////////////////////////////////
///	\file	mapshaderprogram.h
///
///	\author	ELREG
///
///	\brief	shader used for drawing maps.
///
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <QOpenGLShaderProgram>
#include <memory>

#include "../OpenGLBaseLib/shaderprogram.h"
#include "../OpenGLBaseLib/shaderprogramuniform.h"


class CMapShaderProgram : public CShaderProgram
{
public:
	CMapShaderProgram();
	virtual ~CMapShaderProgram();

	void mapShaderSetup( );

	void bind();
	void release();

	void setMVPMatrix(QMatrix4x4 mvp);
	void setResolution(float nWidth, float nHeight);
	void setDashSize(float nDash);
	void setGapSize(float nGap);
	void setDotSize(float nDot);
	void setupVertexState();
	void cleanupVertexState();

private:
	QSharedPointer<CShaderProgramUniform> m_shResolutionLoc;
	QSharedPointer<CShaderProgramUniform> m_shDashSizeLoc;
	QSharedPointer<CShaderProgramUniform> m_shGapSizeLoc;
	QSharedPointer<CShaderProgramUniform> m_shDotSizeLoc;
	QSharedPointer<CShaderProgramUniform> m_shMvpMatrixLoc;


	// Attributes
	GLint m_shVertexLocation;
	GLint m_shColLocation;

};

