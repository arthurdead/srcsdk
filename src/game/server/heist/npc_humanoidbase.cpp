#include "cbase.h"
#include "npc_humanoidbase.h"
#include "heist_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _DEBUG
IMPLEMENT_AUTO_LIST(NPCHumanoidBaseAutoList)
#endif

IMPLEMENT_SERVERCLASS_ST(CNPC_HumanoidBase, DT_NPCHumanoidBase)
	SendPropArray(SendPropFloat(SENDINFO_ARRAY(m_flSuspicion), 16, SPROP_NOSCALE, 0.0f, 100.0f), m_flSuspicion),
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

#ifdef _DEBUG
CON_COMMAND(heist_set_suspicion, "")
{
	if(args.ArgC() != 2) {
		return;
	}

	CHeistPlayer *player = (CHeistPlayer *)UTIL_GetCommandClient();
	if(!player) {
		return;
	}

	float val = V_atof(args.Arg(1));

	auto &list = CNPC_HumanoidBase::AutoList();
	FOR_EACH_VEC(list, i) {
		auto npc = static_cast<CNPC_HumanoidBase *>(list[i]);
		npc->SetSuspicion(player, val);
	}
}
#endif

void CNPC_HumanoidBase::Spawn()
{
	BaseClass::Spawn();

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

void CNPC_HumanoidBase::SetSuspicion(CHeistPlayer *player, float value)
{
	if(!CloseEnough(value, 0.0f) && player->Classify() == CLASS_HEISTER) {
		value = 100.0f;
	}

	int idx = (player->entindex()-1);

	m_flSuspicion.Set(idx, value);

	if(CloseEnough(value, 100.0f) && player->Classify() == CLASS_HEISTER_DISGUISED) {
		player->SetSpotted(true);
	} else if(CloseEnough(value, 0.0f) && player->Classify() == CLASS_HEISTER) {
		player->SetSpotted(false);
	}
}
