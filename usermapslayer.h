#ifndef CUSERMAPSLAYER_H
#define CUSERMAPSLAYER_H

#include "usermapslayerlib_global.h"
#include "../LayerLib/baselayer.h"
#include "../NavUtilsLib/coordinates.h"
#include "usermapsmanager.h"
#include <QTimer>

class USERMAPSLAYERLIB_API CUserMapsLayer : public CBaseLayer
{
	Q_OBJECT

public:
	CUserMapsLayer(QQuickItem *parent = nullptr);
	~CUserMapsLayer();

	QQuickFramebufferObject::Renderer* createRenderer() const override;

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

public slots:
		void selectedObjType(EUserMapObjectType objType);
private:

	void convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints);
	void convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints);
	QVector<CPosition> convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector);
	void onPositionClicked(const QPointF &clickedPosition);
	void setObjectPosition();

	QTimer m_onPressTimer;
	bool m_isMoving;
	QPointF m_startPoint;
	EUserMapObjectType m_objectType;
	QVector<QPointF> m_selectedObjPoints;
	qint64 m_newTime;
	qint64 m_longPressTime;
};

#endif // CUSERMAPSLAYER_H
