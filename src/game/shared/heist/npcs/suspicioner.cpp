#include "cbase.h"
#include "suspicioner.h"
#include "heist_gamerules.h"

#ifdef GAME_DLL
#include "ai_basenpc.h"
#else
#include "c_ai_basenpc.h"
#define CAI_BaseNPC C_AI_BaseNPC
#endif

#ifdef GAME_DLL
#include "heist_player.h"
#else
#include "c_heist_player.h"
#define CHeistPlayer C_HeistPlayer
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
ConVar sv_heist_suspicion_increase_rate("sv_heist_suspicion_increase_rate", "0.02");
ConVar sv_heist_suspicion_decrease_rate("sv_heist_suspicion_decrease_rate", "0.02");
ConVar sv_heist_suspicion_dist("sv_heist_suspicion_dist", "90");
#endif

CUtlVector<CSuspicioner *> CSuspicioner::s_SuspicionerList;

#undef CSuspicioner

#ifdef GAME_DLL
BEGIN_SEND_TABLE_NOBASE(CSuspicioner, DT_Suspicioner)
	SendPropArray(SendPropFloat(SENDINFO_ARRAY(m_flSuspicion), 16, SPROP_NOSCALE, 0.0f, 100.0f), m_flSuspicion)
END_SEND_TABLE()
#else
BEGIN_RECV_TABLE_NOBASE(C_Suspicioner, DT_Suspicioner)
	RecvPropArray(RecvPropFloat(RECVINFO_ARRAYELEM(m_flSuspicion, 0)), m_flSuspicion)
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
	int idx = (player->entindex()-1);

	SetSuspicion(idx, value);
}

void CSuspicioner::SetSuspicion(int idx, float value)
{
	CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(idx);
	if(!pPlayer || !pPlayer->IsAlive()) {
		value = 0.0f;
	} else if((pPlayer && pPlayer->IsSpotted()) || HeistGameRules()->AnyoneSpotted()) {
		value = 100.0f;
	}

	float oldsuspicion = m_flSuspicion.Get(idx);

	if(CloseEnough(oldsuspicion, 0.0f) && !CloseEnough(value, 0.0f)) {
		s_SuspicionerList.AddToTail(this);
	}

	m_flSuspicion.Set(idx, value);

	if(CloseEnough(value, 100.0f) && pPlayer) {
		pPlayer->SetSpotted(true);
		HeistGameRules()->SetSpotted(true);
	}
}
#endif

float CSuspicioner::GetSuspicion(CHeistPlayer *player) const
{
	int idx = (player->entindex()-1);

	return GetSuspicion(idx);
}

float CSuspicioner::GetSuspicion(int idx) const
{
	CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(idx);
	if(!pPlayer || !pPlayer->IsAlive()) {
		return 0.0f;
	} else if((pPlayer && pPlayer->IsSpotted()) || HeistGameRules()->AnyoneSpotted()) {
		return 100.0f;
	}

	return m_flSuspicion.Get(idx);
}

void CSuspicioner::Update()
{
	if(HeistGameRules()->AnyoneSpotted()) {
		return;
	}

	CAI_BaseNPC *pNPC = (CAI_BaseNPC *)m_hOwner.Get();
	if(!pNPC) {
		return;
	}

#ifdef GAME_DLL
	for(int i = 0; i < MAX_PLAYERS; ++i) {
		CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(i+1);
		if(!pPlayer || !pPlayer->IsAlive()) {
			m_flSuspicion.Set(i, 0.0f);
			continue;
		}

		if(pNPC->IsInFieldOfView(pPlayer)) {
			float flNewValue = GetSuspicion(i) + sv_heist_suspicion_increase_rate.GetFloat();
			SetSuspicion(i, flNewValue);
		} else {
			float flNewValue = GetSuspicion(i) + sv_heist_suspicion_decrease_rate.GetFloat();
			SetSuspicion(i, flNewValue);
		}

		DevMsg("%f\n", m_flSuspicion.Get(i));
	}
#endif
}
