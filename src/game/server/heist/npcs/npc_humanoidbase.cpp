#include "cbase.h"
#include "npc_humanoidbase.h"
#include "heist_director.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CNPC_HumanoidBase, DT_NPCHumanoidBase)
END_SEND_TABLE()

CNPC_HumanoidBase::CNPC_HumanoidBase()
	: BaseClass()
{
}

CNPC_HumanoidBase::~CNPC_HumanoidBase()
{
}

void CNPC_HumanoidBase::Spawn()
{
	BaseClass::Spawn();

	CapabilitiesAdd(bits_CAP_TURN_HEAD|bits_CAP_ANIMATEDFACE);

	SetBloodColor(BLOOD_COLOR_RED);

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetMoveType(MOVETYPE_STEP);
	CapabilitiesAdd(
		bits_CAP_MOVE_GROUND|
		bits_CAP_DUCK|
		bits_CAP_MOVE_JUMP|
		bits_CAP_MOVE_CLIMB|
		bits_CAP_DOORS_GROUP
	);

	m_NPCState = NPC_STATE_NONE;
	SetHealth( 10 );
	m_flFieldOfView = 0.5f;
	NPCInit();

	if(MissionDirector()->GetMissionState() == MISSION_STATE_CASING) {
		SetContextThink(&CNPC_HumanoidBase::SuspicionThink, gpGlobals->curtime + 0.2f, "SuspicionThink");
	}
}

void CNPC_HumanoidBase::SuspicionThink()
{
	if(MissionDirector()->GetMissionState() != MISSION_STATE_CASING) {
		SetNextThink(TICK_NEVER_THINK, "SuspicionThink");
		return;
	}

	

	SetNextThink(gpGlobals->curtime + 0.2f, "SuspicionThink");
}
