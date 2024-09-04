#include "cbase.h"
#include "npc_humanoidbase.h"
#include "heist_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CNPC_HumanoidBase, DT_NPCHumanoidBase)
	SendPropDataTable(SENDINFO_DT(m_Suspicioner), &REFERENCE_SEND_TABLE(DT_Suspicioner)),
END_SEND_TABLE()

AI_BEGIN_CUSTOM_NPC(ignored, CNPC_HumanoidBase)
	DECLARE_CONDITION(COND_UNCOVERED_HEISTER)

	DEFINE_SCHEDULE(
		SCHED_SCAN_FOR_SUSPICIOUS_ACTIVITY,
R"(

Tasks
	TASK_FIND_DODGE_DIRECTION 3
	TASK_JUMP 0

Interrupts
	COND_LIGHT_DAMAGE

)"
	)

AI_END_CUSTOM_NPC()

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

	m_Suspicioner.Init(this);

	CapabilitiesAdd(bits_CAP_TURN_HEAD|bits_CAP_ANIMATEDFACE);

	SetBloodColor(BLOOD_COLOR_RED);

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetMoveType(MOVETYPE_STEP);
	CapabilitiesAdd(bits_CAP_MOVE_GROUND|bits_CAP_DUCK|bits_CAP_MOVE_JUMP|bits_CAP_MOVE_CLIMB|bits_CAP_DOORS_GROUP);

	m_NPCState = NPC_STATE_NONE;
	SetHealth( 10 );
	m_flFieldOfView = 0.5f;
	NPCInit();
}
