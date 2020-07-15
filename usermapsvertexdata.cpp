////////////////////////////////////////////////////////////////////////////////
///	\file	usermapsvertexdata.cpp
///
///	\author	ELREG
///
///	\brief	definition of the class that holds generic vertex data and linestyle
///			.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////
#include "usermapsvertexdata.h"

CUserMapsVertexData::CUserMapsVertexData()
{

}

float CUserMapsVertexData::getDashSize()const {
	return m_DashSize;
}

void CUserMapsVertexData::setDashSize(float dashSize) {
	 m_DashSize=dashSize;
}

float CUserMapsVertexData::getDotSize()const {
	return m_DotSize;
}

void CUserMapsVertexData::setDotSize(float dotSize) {
	 m_DotSize=dotSize;
}

float CUserMapsVertexData::getGapSize()const {
	return m_GapSize;
}

void CUserMapsVertexData::setGapSize(float gapSize) {
	 m_GapSize=gapSize;
}

std::vector<GenericVertexData> CUserMapsVertexData::getVertexData() const {
	return m_pVertexData;
}

void CUserMapsVertexData::setVertexData(std::vector<GenericVertexData> vertexData) {
	m_pVertexData=vertexData;
}
