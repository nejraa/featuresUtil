/////////////////////////////////////////////////////////////////////////////////
///	\file   usermapslayer.h
///
///	\author Elreg
///
///	\brief	Declaration of the CUserMapsLayer class which represents
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2020.
/////////////////////////////////////////////////////////////////////////////////

#ifndef CUSERMAPSLAYER_H
#define CUSERMAPSLAYER_H

#include "usermapslayerlib_global.h"
#include "../LayerLib/baselayer.h"
#include "usermapsmanager.h"
#include "userpointpositiontype.h"
#include <QTimer>

////////////////////////////////////////////////////////////////////////////////
/// \brief CUserMapsLayer - class which represents the user maps layer.
////////////////////////////////////////////////////////////////////////////////
class USERMAPSLAYERLIB_API CUserMapsLayer : public CBaseLayer
{
	Q_OBJECT

public:
	CUserMapsLayer(QQuickItem *parent = nullptr);
	~CUserMapsLayer();

	QQuickFramebufferObject::Renderer* createRenderer() const override;

public slots:
	void onOffsetChanged();

	void setSelectedObject(bool isObjSelected, EUserMapObjectType objType);

protected:
	void initialise() override;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
	void pressTimerTimeout();

private:
	// Coordinate conversion functions
	static void convertGeoVectorToPixelVector(const QVector<CPosition> &geoPoints, QVector<QPointF> &pixelPoints);
	static void convertGeoPointToPixelVector(const CPosition &geoPoint, QVector<QPointF> &pixelPoints);
	static void convertPixelVectorToGeoVector(const QVector<QPointF> &pixelVector, QVector<CPosition> &geoPoints);
	static CPosition convertPixelPointToGeoPoint(const QPointF &pixelPoint);
	static QPointF convertGeoPointToPixelPoint(const CPosition &geoPoint);

	void createManagerConnections();

	// Object manipulation
	void onPositionClicked(const QPointF &clickedPosition);
	void updateObjectPosition();

	// Handling press/move events
	void handleObjAction(const QPointF &initialPosition, const QPointF &endPosition);
	void handleAreaObjAction(const QPointF &initialPosition, const QPointF &endPosition);
	void handleLineObjAction(const QPointF &initialPosition, const QPointF &endPosition);
	void handleCircleObjAction(const QPointF &initialPosition, const QPointF &endPosition);
	void handlePointObjAction(const QPointF &initialPosition, const QPointF &endPosition);
	void moveObj(const QPointF &initialPosition, const QPointF &endPosition);
	void moveObjPoint(const QPointF &initialPosition, const QPointF &endPosition, const int index);
	void moveObjPoints(const QPointF &initialPosition, const QPointF &endPosition, const int index1, const int index2);
	void deleteObjPoint(const int index);
	void insertObjPoint(const int index, const QPointF &pos);

	// Position estimation of clicked point
	EPointPositionType checkPointPositionToObj (const QPointF &clickedPosition, int &index1, int &index2);
	EPointPositionType pointPositionToArea(const QPointF &clickedPosition, int &index1, int &index2);
	EPointPositionType pointPositionToCircle(const QPointF &clickedPosition);
	EPointPositionType pointPositionToLine(const QPointF &clickedPosition, int &index1, int &index2);
	EPointPositionType pointPositionToPointObj(const QPointF &clickedPosition);
	qreal calculateYaxisValueOnLine(const QPointF &pointA, const QPointF &pointB, const QPointF clickedPoint);

	QTimer m_onPressTimer;                   ///< Timer for press event.
	bool m_isCursorMoving;                   ///< Flag describing whether the cursor is moving or not.
	bool m_isLongMousePress;                 ///< Flag describing whether long mouse press occurred or not.
	QPointF m_moveEvtStartPoint;             ///< Move event start point in pixels.
	EUserMapObjectType m_objectType;         ///< Type of object.
	QVector<QPointF> m_selectedObjPoints;    ///< Vector of points of selected object.
	qint64 m_moveEvtTimestamp;               ///< Timestamp for move event.
	EPointPositionType  m_pointPositionType; ///< Type of clicked point position.
	int m_index1;                            ///< Index of the first point on line segment of area/line object where clicked position lies.
	int m_index2;                            ///< Index of the second point on line segment of area/line object where clicked position lies.
};

#endif // CUSERMAPSLAYER_H
