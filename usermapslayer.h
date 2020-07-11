////////////////////////////////////////////////////////////////////////////////
///	\file   usermapslayer.h
///
///	\author Elreg
///
///	\brief	Declaration of the CUserMapsLayer class which represents
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////

#ifndef CUSERMAPSLAYER_H
#define CUSERMAPSLAYER_H

#include "usermapslayerlib_global.h"
#include "../LayerLib/baselayer.h"
#include "../NavUtilsLib/coordinates.h"
#include "usermapsmanager.h"
#include "userpointpositiontype.h"
#include <QTimer>

class USERMAPSLAYERLIB_API CUserMapsLayer : public CBaseLayer
{
	Q_OBJECT

public:
	CUserMapsLayer(QQuickItem *parent = nullptr);
	~CUserMapsLayer();

	QQuickFramebufferObject::Renderer* createRenderer() const override;    
    EPointPositionType checkPointPosition (const QPointF &clickedPosition);

public slots:
	void setSelectedObject(bool isObjSelected, EUserMapObjectType objType);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
	void pressTimerTimeout();

private:
	static void convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints);
	static void convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints);
	static void convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector, QVector<CPosition> &geoPoints);
	static CPosition convertPixelPointToGeoPoint(const QPointF &pixelPoint);
    static QPointF convertGeoPointToPixelPoint(const CPosition &geoPoint);
	void onPositionClicked(const QPointF &clickedPosition);
	void updateObjectPosition();
    EPointPositionType isInsideArea(const QPointF &clickedPosition);
    EPointPositionType isInsideCircle(const QPointF &clickedPosition);
    EPointPositionType isOnLine(const QPointF &clickedPosition);


	QTimer m_onPressTimer;
	bool m_isCursorMoving;
	QPointF m_moveEvtStartPoint;
	EUserMapObjectType m_objectType;
    QVector<QPointF> m_selectedObjPoints;
    // cirle = centralna tacka
    // liniju = sve tacke area = sve tacke
	qint64 m_moveEvtTimestamp;
};

#endif // CUSERMAPSLAYER_H
