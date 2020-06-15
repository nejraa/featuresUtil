#ifndef CUSERMAPSLAYER_H
#define CUSERMAPSLAYER_H

#include "usermapslayerlib_global.h"
#include "../LayerLib/baselayer.h"
#include "../NavUtilsLib/coordinates.h"
#include "usermapsmanager.h"
#include <QTimer>
#include <QSharedPointer>

class USERMAPSLAYERLIB_API CUserMapsLayer : public CBaseLayer
{
	Q_OBJECT

public:
	CUserMapsLayer(QQuickItem *parent = nullptr);
	~CUserMapsLayer();

	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

	QQuickFramebufferObject::Renderer* createRenderer() const override;


public slots:
		void selectedObjType(EUserMapObjectType objType);
private:

	void convertGeoVectorToPixelVector(QVector<CPosition> objPoints);
	void convertGeoPointToPixelVector(CPosition objPoint);
	QVector<CPosition> convertPixelVectorToGeoVector();
	void setObjectPosition();

	QTimer m_onPressTimer;
	bool m_isMoving;
	QVector<QPointF> m_selectedObjPoints;
	QPointF m_startPoint;
	EUserMapObjectType m_objectType;
};

#endif // CUSERMAPSLAYER_H
