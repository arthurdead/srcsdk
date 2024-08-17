#include "cbase.h"
#include "npc_civilian.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(npc_civilian, CNPC_Civilian);

IMPLEMENT_CUSTOM_AI(npc_civilian, CNPC_Civilian);

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

void CNPC_Civilian::InitCustomSchedules() 
{
	INIT_CUSTOM_AI(CNPC_Civilian);
}

void CNPC_Civilian::Spawn()
{
	BaseClass::Spawn();

	if(GetModelName() == NULL_STRING) {
		SetModel(g_pszCivilianModels[random->RandomInt(0, ARRAYSIZE(g_pszCivilianModels)-1)]);
	}

	CapabilitiesAdd(bits_CAP_TURN_HEAD|bits_CAP_ANIMATEDFACE);
	SetBloodColor(BLOOD_COLOR_RED);

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetMoveType(MOVETYPE_STEP);
	CapabilitiesAdd(bits_CAP_MOVE_GROUND|bits_CAP_OPEN_DOORS);

	m_NPCState = NPC_STATE_NONE;
	m_iHealth = 10;
	m_flFieldOfView = 0.5f;
	NPCInit();
}
