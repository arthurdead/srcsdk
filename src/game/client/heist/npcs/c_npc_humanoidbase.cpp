#include "cbase.h"
#include "c_npc_humanoidbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_NPC_HumanoidBase, DT_NPCHumanoidBase, CNPC_HumanoidBase)
END_RECV_TABLE()

C_NPC_HumanoidBase::C_NPC_HumanoidBase()
	: BaseClass()
{
}

void C_NPC_HumanoidBase::Spawn()
{
	BaseClass::Spawn();
}
