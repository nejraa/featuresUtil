////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsobjectutil.h
///
///	\author Elreg
///
///	\brief	Definition of the CUserMapsObjectUtil class which provides
///			required functionalities for object manipulation.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#ifndef CUSERMAPSOBJECTUTIL_H
#define CUSERMAPSOBJECTUTIL_H

#include <QObject>
#include <QVector>
// #include "usermapslayer.h"
#include "usermapobject.h"
// #include "usermapslayerlib_global.h"
// #include "usermaparea.h"
// #include "usermapcircle.h"
// #include "usermapline.h"
// #include "usermappoint.h"
#include "usermapobjecttype.h"

// class EUserMapObjectType;
// class CPosition;
// class CUserMapObject;

class CUserMapsObjectUtil
{
public:
    CUserMapsObjectUtil(CUserMapObject *pObjectSelected);
    ~CUserMapsObjectUtil();

    bool checkPointPosition (const QPointF &pixelPoint, CUserMapObject *pObject, EUserMapObjectType objType);

    // CUserMapObject base class
    // CUserMapCircle (public CUserMapComplexObject) CPosition m_center; float m_radius;
    // CUserMapPoint CPosition m_position;
    // CUserMapArea vector of points (public CUserMapComplexObject) QVector<CPosition>
    // CUserMapLine vector of points (public CUserMapComplexObject) QVector<CPosition>


private:
    // might be also QPointF instead CPosition
    // static void convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints);
    // static void convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints);
    // static void convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector, QVector<CPosition> &geoPoints);
    // static CPosition convertPixelPointToGeoPoint(const QPointF &pixelPoint);

    bool isInsideArea(const CPosition &position, CUserMapObject *pObject);
    bool isInsideCircle(const CPosition &position, CUserMapObject *pObject);
    bool isOnLine(const CPosition &position, CUserMapObject *pObject);

    CUserMapObject *m_pObject;  ///< Pointer to the selected object.
    // EUserMapObjectType m_objectType;

};

#endif // CUSERMAPSOBJECTUTIL_H
