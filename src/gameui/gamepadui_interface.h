#ifndef GAMEPADUI_INTERFACE_H
#define GAMEPADUI_INTERFACE_H
#pragma once

#include "tier0/platform.h"
#include "gamepadui/igamepadui.h"

class GamepadUI : public IGamepadUI
{
public:
	static GamepadUI& GetInstance();

	void Initialize( CreateInterfaceFn factory ) OVERRIDE;
	void Shutdown() OVERRIDE;

	void OnUpdate( float flFrametime ) OVERRIDE;
	void OnLevelInitializePreEntity() OVERRIDE;
	void OnLevelInitializePostEntity() OVERRIDE;
	void OnLevelShutdown() OVERRIDE;

	void VidInit() OVERRIDE;

private:
	static GamepadUI *s_pGamepadUI;
};

#endif // GAMEPADUI_INTERFACE_H
