//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

#ifndef ISHADEREXTENSION_H
#define ISHADEREXTENSION_H
#pragma once

#include "materialsystem/imaterialsystem.h"
#include "appframework/IAppSystem.h"

class IShaderExtension : public IAppSystem
{
protected:
	virtual ~IShaderExtension() {};

public:
	virtual void SetUberlightParamsForFlashlightState( FlashlightState_t&, const UberlightState_t & ) = 0;
	virtual void OnFlashlightStateDestroyed( const FlashlightState_t& ) = 0;
};

#define SHADEREXTENSION_INTERFACE_VERSION "IShaderExtension001"

#endif // ISHADEREXTENSION_H