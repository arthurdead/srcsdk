#include "cbase.h"
#include "npc_civilian.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(npc_civilian, CNPC_Civilian);

static const char *g_pszCivilianModels[]{
	"models/humans/group01/male_01.mdl"
};

void CNPC_Civilian::Precache()
{
	BaseClass::Precache();

	for(int i = 0; i < ARRAYSIZE(g_pszCivilianModels); ++i) {
		PrecacheModel(g_pszCivilianModels[i]);
	}
}

void CNPC_Civilian::Spawn()
{
	if(GetModelName() == NULL_STRING) {
		SetModel(g_pszCivilianModels[random->RandomInt(0, ARRAYSIZE(g_pszCivilianModels)-1)]);
	}

	UTIL_SetSize(this, NAI_Hull::Mins(RECAST_NAVMESH_HUMAN), NAI_Hull::Maxs(RECAST_NAVMESH_HUMAN));

	CapabilitiesAdd(
		bits_CAP_NO_HIT_PLAYER
	);

	ChangeTeam( TEAM_CIVILIANS );
	ChangeFaction( FACTION_CIVILIANS );

	BaseClass::Spawn();
}
