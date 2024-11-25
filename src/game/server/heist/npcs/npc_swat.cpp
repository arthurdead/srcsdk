#include "cbase.h"
#include "npc_swat.h"
#include "heist_player.h"
#include "heist_gamerules.h"
#include "npcevent.h"
#include "heist_director.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//TEMP while developing remove later
LINK_ENTITY_TO_CLASS(npc_metropolice, CNPC_Swat);

LINK_ENTITY_TO_CLASS(npc_swat, CNPC_Swat);

BEGIN_MAPENTITY(CNPC_Swat, MAPENT_NPCCLASS)
	DEFINE_KEYFIELD( m_bHeavy, FIELD_BOOLEAN, "heavy" )
END_MAPENTITY()

static const char *g_pszCopModels[]{
	"models/police.mdl"
};

void CNPC_Swat::Precache()
{
	BaseClass::Precache();

	for(int i = 0; i < ARRAYSIZE(g_pszCopModels); ++i) {
		PrecacheModel(g_pszCopModels[i]);
	}
}

void CNPC_Swat::Spawn()
{
	if(GetModelName() == NULL_STRING) {
		SetModel(g_pszCopModels[random_valve->RandomInt(0, ARRAYSIZE(g_pszCopModels)-1)]);
	}

	ChangeTeam( TEAM_POLICE );
	ChangeFaction( FACTION_APEX_SECURITY );

	CBaseCombatWeapon *pWeapon = NULL;
	if(m_bHeavy) {
		pWeapon = GiveWeapon( MAKE_STRING("weapon_hl2_shotgun") );
	} else {
		pWeapon = GiveWeapon( MAKE_STRING("weapon_hl2_pistol") );
	}
	if( pWeapon )
		GiveAmmo(99, pWeapon->m_iPrimaryAmmoType);

	BaseClass::Spawn();
}

Class_T CNPC_Swat::Classify()
{
	return CLASS_POLICE;
}
