#ifndef CUSERMAPSLAYER_H
#define CUSERMAPSLAYER_H

#include "usermapslayerlib_global.h"
#include "../LayerLib/baselayer.h"

class USERMAPSLAYERLIB_API CUserMapsLayer : public CBaseLayer
{
	Q_OBJECT

public:
	CUserMapsLayer(QQuickItem *parent = nullptr);
	~CUserMapsLayer();

	QQuickFramebufferObject::Renderer* createRenderer() const override;
};

#endif // CUSERMAPSLAYER_H
