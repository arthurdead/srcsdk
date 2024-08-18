#ifndef NPC_HUMANOIDBASE_H
#define NPC_HUMANOIDBASE_H

#pragma once

#include "ai_baseactor.h"
#include "ai_heist.h"

class CHeistPlayer;

#ifdef _DEBUG
DECLARE_AUTO_LIST(NPCHumanoidBaseAutoList);
#endif

class CNPC_HumanoidBase : public CAI_BaseActor
#ifdef _DEBUG
	,public NPCHumanoidBaseAutoList
#endif
{
public:
	DECLARE_CLASS(CNPC_HumanoidBase, CAI_BaseActor);
	DECLARE_SERVERCLASS();

	CNPC_HumanoidBase();
	~CNPC_HumanoidBase() override;

	void Spawn() override;

	DEFINE_CUSTOM_AI;

	void SetSuspicion(CHeistPlayer *pPlayer, float value);

private:
	CNetworkArray(float, m_flSuspicion, MAX_PLAYERS);
};

#endif
