#include "game_loopback/igameloopback.h"
#include "tier1/interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGameLoopback : public CBaseAppSystem< IGameLoopback >
{

};

static CGameLoopback g_GameLookpback;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameLoopback, IGameLoopback, GAMELOOPBACK_INTERFACE_VERSION, g_GameLookpback);
