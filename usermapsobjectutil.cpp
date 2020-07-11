////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsobjectutil.cpp
///
///	\author Elreg
///
///	\brief	Implementation of the CUserMapsObjectUtil class
///         with provided functionalities for object manipulation.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#include "usermapsobjectutil.h"


CUserMapsObjectUtil::CUserMapsObjectUtil(CUserMapObject *pObjectSelected):
    m_pObject(pObjectSelected)
{

}

bool CUserMapsObjectUtil::checkPointPosition(const QPointF &pixelPoint, CUserMapObject *pObject, EUserMapObjectType objType)
{
    /*CPosition pos = convertPixelPointToGeoPoint(pixelPoint);
    switch (objType)
        {
        case EUserMapObjectType::Area:
            return isInsideArea(pos, pObject);
        case EUserMapObjectType::Circle:
            return isInsideCircle(pos, pObject);
        case EUserMapObjectType::Line:
            return isOnLine(pos, pObject);
        case EUserMapObjectType::Point:
            return true;
        case EUserMapObjectType::Unkown_Object:
            return false;
    }*/
    return false; // TO DO
}



bool CUserMapsObjectUtil::isInsideArea(const CPosition &position, CUserMapObject *pObject)
{

    return false;
}

bool CUserMapsObjectUtil::isInsideCircle(const CPosition &position, CUserMapObject *pObject)
{
    return false;
}

bool CUserMapsObjectUtil::isOnLine(const CPosition &position, CUserMapObject *pObject)
{
    return false;
}

