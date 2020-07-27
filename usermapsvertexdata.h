////////////////////////////////////////////////////////////////////////////////
///	\file	usermapsvertexdata.h
///
///	\author	ELREG
///
///	\brief	definition of the class that holds generic vertex data and linestyle.
///
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////
#ifndef USERMAPSVERTEXDATA_H
#define USERMAPSVERTEXDATA_H

#include <vector>
#include "baserenderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include "../OpenGLBaseLib/imagetexture.h"
#include "../OpenGLBaseLib/vertexbuffer.h"
class CUserMapsVertexData
{
public:
	CUserMapsVertexData();
	float getDashSize() const;
	void setDashSize(float dashSize);
	float getDotSize() const;
	void setDotSize(float dotSize);
	float getGapSize() const;
	void setGapSize(float gapSize);
	float GetLineWidth() const;
	void setLineWidth(float lineWidth);

	std::vector<GenericVertexData> getVertexData() const;
	void setVertexData(std::vector<GenericVertexData> vertexData);
	void addVertexData(GenericVertexData vertexData);

private:
	std::vector<GenericVertexData> m_pVertexData; //colour and position data
	float m_DashSize;///< dash size of the line
	float m_DotSize; ///<dot size of the line(1 if it is present and 0 if it is not
	float m_GapSize;///<gap between elements
	float m_LineWidth;///<gap between elements
};

#endif // USERMAPSVERTEXDATA_H
