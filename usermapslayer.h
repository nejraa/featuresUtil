////////////////////////////////////////////////////////////////////////////////
///	\file   usermapsrenderer.h
///
///	\author Elreg
///
///	\brief	Declaration of the CUserMapsLayer class which represents
///			the user maps layer.
///
///	(C) Kelvin Hughes, 2019.
////////////////////////////////////////////////////////////////////////////////

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

public slots:
    void onOffsetChanged();
protected:
    virtual void initialise() override;

};

#endif // CUSERMAPSLAYER_H
