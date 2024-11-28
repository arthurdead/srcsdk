#include "gamepadui_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_GAMEPADUI, "GamepadUI", LCF_NONE, LS_MESSAGE, Color( 255, 134, 44, 255 ) );

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( GamepadUI, IGamepadUI, GAMEPADUI_INTERFACE_VERSION, GamepadUI::GetInstance() );

GamepadUI *GamepadUI::s_pGamepadUI = NULL;

GamepadUI& GamepadUI::GetInstance()
{
	if ( !s_pGamepadUI )
		s_pGamepadUI = new GamepadUI;

	return *s_pGamepadUI;
}

#ifdef __MINGW32__
void GamepadUI::__DTOR__()
{
	this->~GamepadUI();
}
#endif

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
