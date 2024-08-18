#include "cbase.h"
#include "npc_cop.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(npc_cop, CNPC_Cop);

IMPLEMENT_CUSTOM_AI(npc_cop, CNPC_Cop);

static const char *g_pszCopModels[]{
	"models/police.mdl"
};

void CNPC_Cop::Precache()
{
	BaseClass::Precache();

	for(int i = 0; i < ARRAYSIZE(g_pszCopModels); ++i) {
		PrecacheModel(g_pszCopModels[i]);
	}
}

void CNPC_Cop::InitCustomSchedules() 
{
	INIT_CUSTOM_AI(CNPC_Cop);
}

void CNPC_Cop::Spawn()
{
	if(GetModelName() == NULL_STRING) {
		SetModel(g_pszCopModels[random->RandomInt(0, ARRAYSIZE(g_pszCopModels)-1)]);
	}

	BaseClass::Spawn();
}

void CNPC_Cop::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);

	if(pOther->IsPlayer() && pOther->Classify() == CLASS_HEISTER_DISGUISED) {
		SetSuspicion((CHeistPlayer *)pOther, 100.0f);
	}
}
