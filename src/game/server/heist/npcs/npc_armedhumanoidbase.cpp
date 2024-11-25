#include "cbase.h"
#include "npc_armedhumanoidbase.h"
#include "heist_player.h"
#include "heist_gamerules.h"
#include "npcevent.h"
#include "heist_director.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CNPC_ArmedHumanoidBase::Spawn()
{
	UTIL_SetSize(this, NAI_Hull::Mins(RECAST_NAVMESH_HUMAN), NAI_Hull::Maxs(RECAST_NAVMESH_HUMAN));

	CapabilitiesAdd(
		bits_CAP_NO_HIT_SQUADMATES|
		bits_CAP_SQUAD|
		bits_CAP_AIM_GUN|
		bits_CAP_MOVE_SHOOT|
		bits_CAP_USE_WEAPONS
	);

	BaseClass::Spawn();

	SetTouch(&CNPC_ArmedHumanoidBase::HeisterTouch);
}

void CNPC_ArmedHumanoidBase::HeisterTouch(CBaseEntity *pOther)
{
	if(MissionDirector()->GetMissionState() != MISSION_STATE_CASING)
		return;

	if(pOther->IsPlayer()) {
		MissionDirector()->MakeMissionLoud();
	}
}

void CNPC_ArmedHumanoidBase::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);
	HeisterTouch(pOther);
}
