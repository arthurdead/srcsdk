#include "cbase.h"
#include "heist_player_shared.h"
#include "heist_gamerules.h"

#ifdef CLIENT_DLL
#include "c_heist_player.h"
#else
#include "heist_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CHeistPlayer::IsSpotted() const
{
	return (m_bSpotted || HeistGameRules()->AnyoneSpotted());
}
