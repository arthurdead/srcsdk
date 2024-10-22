#ifndef IGAMEPADUI_H
#define IGAMEPADUI_H
#pragma once

#include "tier1/interface.h"
#include "vgui/VGUI.h"
#include "mathlib/vector.h"
#include "ivrenderview.h"

DECLARE_LOGGING_CHANNEL( LOG_GAMEPADUI );

abstract_class IGamepadUI : public IBaseInterface
{
public:
	virtual void Initialize( CreateInterfaceFn factory ) = 0;
	virtual void Shutdown() = 0;

	virtual void OnUpdate( float flFrametime ) = 0;
	virtual void OnLevelInitializePreEntity() = 0;
	virtual void OnLevelInitializePostEntity() = 0;
	virtual void OnLevelShutdown() = 0;

	virtual void VidInit() = 0;
};

#define GAMEPADUI_INTERFACE_VERSION "GamepadUI001"

#define GamepadUI_Msg(...) Log_Msg( LOG_GAMEPADUI, "[GamepadUI] " __VA_ARGS__ )

#define GamepadUI_Log(...) GamepadUI_Msg( __VA_ARGS__ )

#endif // IGAMEPADUI_H
