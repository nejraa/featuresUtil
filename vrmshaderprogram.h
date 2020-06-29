#pragma once

#include <QOpenGLShaderProgram>
#include <memory>

#include "../OpenGLBaseLib/shaderprogram.h"
#include "../OpenGLBaseLib/shaderprogramuniform.h"

class CVRMShaderProgram : public CShaderProgram
{
public:
    CVRMShaderProgram();
    virtual ~CVRMShaderProgram();

    void vrmShaderSetup();

    void bind();
    void release();

    void setMVPMatrix(QMatrix4x4 mvp);
    void setResolution(float nWidth, float nHeight);
    void setDashSize(float nDash);
    void setGapSize(float nGap);

    void setupVertexState();
    void cleanupVertexState();

private:
     QSharedPointer<CShaderProgramUniform> m_shResolutionLoc;
     QSharedPointer<CShaderProgramUniform> m_shDashSizeLoc;
     QSharedPointer<CShaderProgramUniform> m_shGapSizeLoc;
     QSharedPointer<CShaderProgramUniform> m_shMvpMatrixLoc;

     // Attributes
     GLint m_shVertexLocation;
     GLint m_shColLocation;
     GLint m_shDistanceLocation;

};
