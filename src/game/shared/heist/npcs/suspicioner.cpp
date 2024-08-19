#include "cbase.h"
#include "suspicioner.h"
#include "heist_gamerules.h"

#ifdef GAME_DLL
#include "heist_player.h"
#else
#include "c_heist_player.h"
#define CHeistPlayer C_HeistPlayer
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUtlVector<CSuspicioner *> CSuspicioner::s_SuspicionerList;

#undef CSuspicioner

#ifdef GAME_DLL
BEGIN_SEND_TABLE_NOBASE(CSuspicioner, DT_Suspicioner)
	SendPropArray(SendPropFloat(SENDINFO_ARRAY(m_flSuspicion), 16, SPROP_NOSCALE, 0.0f, 100.0f), m_flSuspicion)
END_SEND_TABLE()
#else
BEGIN_RECV_TABLE_NOBASE(C_Suspicioner, DT_Suspicioner)
	RecvPropArray(RecvPropFloat(RECVINFO(m_flSuspicion[0])), m_flSuspicion)
END_RECV_TABLE()
#endif

#ifdef CLIENT_DLL
#define CSuspicioner C_Suspicioner
#endif

CSuspicioner::CSuspicioner()
{
	m_hOwner = NULL;
}

CSuspicioner::~CSuspicioner()
{
	s_SuspicionerList.FindAndFastRemove(this);
}

void CSuspicioner::Init(CBaseEntity *owner)
{
	m_hOwner = owner;
}

#ifdef GAME_DLL
void CSuspicioner::SetSuspicion(CHeistPlayer *player, float value)
{
	if(player->IsSpotted()) {
		return;
	}

	int idx = (player->entindex()-1);

	float oldsuspicion = m_flSuspicion.Get(idx);

	if(CloseEnough(oldsuspicion, 0.0f) && !CloseEnough(value, 0.0f)) {
		s_SuspicionerList.AddToTail(this);
	}

	m_flSuspicion.Set(idx, value);

	if(CloseEnough(value, 100.0f)) {
		player->SetSpotted(true);
	}
}
#endif

float CSuspicioner::GetSuspicion(CHeistPlayer *player) const
{
	if(player->IsSpotted()) {
		return 100.0f;
	}

	int idx = (player->entindex()-1);

	return m_flSuspicion.Get(idx);
}
