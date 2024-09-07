#include "cbase.h"
#include "heist_player.h"
#include "heist_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(player, CHeistPlayer);

BEGIN_SEND_TABLE_NOBASE(CHeistPlayer, DT_HeistLocalPlayerExclusive)
	SendPropBool(SENDINFO(m_bSpotted)),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE(CHeistPlayer, DT_HeistNonLocalPlayerExclusive)
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CHeistPlayer, DT_Heist_Player)
	SendPropDataTable("heistlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HeistLocalPlayerExclusive), SendProxy_SendLocalDataTable),
	SendPropDataTable("heistnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HeistNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable),
END_SEND_TABLE()

CHeistPlayer::CHeistPlayer()
	: BaseClass()
{
}

CHeistPlayer::~CHeistPlayer()
{
}

void CHeistPlayer::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
}

void CHeistPlayer::Precache()
{
	BaseClass::Precache();

	PrecacheModel("models/player/leader.mdl");
}

void CHeistPlayer::SetSpotted(bool value)
{
	m_bSpotted = value;

	if(value) {
		for(int i = 1; i <= gpGlobals->maxClients; ++i) {
			CHeistPlayer *other = (CHeistPlayer *)UTIL_PlayerByIndex(i);
			if(!other) {
				continue;
			}

			other->m_bSpotted = true;
		}

		HeistGameRules()->SetSpotted(true);

		ChangeTeam( TEAM_HEISTERS );
		ChangeFaction( FACTION_HEISTERS );
	} else {
		bool any_spotted = false;

		for(int i = 1; i <= gpGlobals->maxClients; ++i) {
			CHeistPlayer *other = (CHeistPlayer *)UTIL_PlayerByIndex(i);
			if(!other) {
				continue;
			}

			if(other->m_bSpotted) {
				any_spotted = true;
				break;
			}
		}

		if(!any_spotted) {
			HeistGameRules()->SetSpotted(false);
		}

		ChangeTeam( TEAM_CIVILIANS );
		ChangeFaction( FACTION_CIVILIANS );
	}
}

void CHeistPlayer::Spawn()
{
	ChangeTeam( TEAM_CIVILIANS );
	ChangeFaction( FACTION_CIVILIANS );

	BaseClass::Spawn();

	SetModel("models/player/leader.mdl");
}

Class_T CHeistPlayer::Classify()
{
	return IsSpotted() ? CLASS_HEISTER : CLASS_CIVILIAN;
}
