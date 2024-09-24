#include "gamepadui_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( GamepadUI, IGamepadUI, GAMEPADUI_INTERFACE_VERSION, GamepadUI::GetInstance() );

GamepadUI *GamepadUI::s_pGamepadUI = NULL;

GamepadUI& GamepadUI::GetInstance()
{
	if ( !s_pGamepadUI )
		s_pGamepadUI = new GamepadUI;

	return *s_pGamepadUI;
}

void GamepadUI::Initialize( CreateInterfaceFn factory )
{
}

void GamepadUI::Shutdown()
{
}

void GamepadUI::OnUpdate( float flFrametime )
{
}

void GamepadUI::OnLevelInitializePreEntity()
{
}

void GamepadUI::OnLevelInitializePostEntity()
{
}

void GamepadUI::OnLevelShutdown()
{
}

void GamepadUI::VidInit()
{
}
