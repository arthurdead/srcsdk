#include "cbase.h"
#include "c_npc_humanoidbase.h"
#include "debugoverlay_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_NPC_HumanoidBase, DT_NPCHumanoidBase, CNPC_HumanoidBase)
	RecvPropArray(RecvPropFloat(RECVINFO(m_flSuspicion[0])), m_flSuspicion)
END_RECV_TABLE()

C_NPC_HumanoidBase::C_NPC_HumanoidBase()
	: BaseClass()
{
}

int C_NPC_HumanoidBase::DrawModel(int flags)
{
	int ret = BaseClass::DrawModel(flags);

	static float last_print = 0.0f;
	if(last_print <= gpGlobals->curtime) {
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		if(player) {
			char szSuspicion[256];
			V_sprintf_safe(szSuspicion, "%.1f", m_flSuspicion[player->entindex()-1]);
			NDebugOverlay::EntityText(entindex(), 0, szSuspicion, 0.1f);
		}
		last_print = gpGlobals->curtime + 0.1f;
	}

	return ret;
}
