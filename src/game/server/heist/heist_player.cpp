#include "cbase.h"
#include "heist_player.h"
#include "heist_director.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(player, CHeistPlayer);

BEGIN_SEND_TABLE_NOBASE(CHeistPlayer, DT_HeistLocalPlayerExclusive)
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

void CHeistPlayer::Spawn()
{
	ChangeTeam( TEAM_CIVILIANS, false, false );
	ChangeFaction( FACTION_CIVILIANS );

	SetModel( "models/player/leader.mdl" );

	BaseClass::Spawn();

	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon *)GiveNamedItem( "weapon_hl2_pistol", 0, false );
	if( pWeapon ) {
		GiveAmmo(99, pWeapon->m_iPrimaryAmmoType);
	}

	pWeapon = (CBaseCombatWeapon *)GiveNamedItem( "weapon_hl2_shotgun", 0, false );
	if( pWeapon ) {
		GiveAmmo(99, pWeapon->m_iPrimaryAmmoType);
	}

	pWeapon = (CBaseCombatWeapon *)GiveNamedItem( "weapon_hl2_smg1", 0, false );
	if( pWeapon ) {
		GiveAmmo(99, pWeapon->m_iPrimaryAmmoType);
	}

	MissionDirector()->PlayerSpawned(this);
}

Class_T CHeistPlayer::Classify()
{
	return MissionDirector()->IsMissionLoud() ? CLASS_HEISTER : CLASS_CIVILIAN;
}
