#ifndef NPC_ARMEDHUMANOIDBASE_H
#define NPC_ARMEDHUMANOIDBASE_H

#pragma once

#include "npc_humanoidbase.h"
#include "heist_shareddefs.h"

class CNPC_ArmedHumanoidBase : public CNPC_HumanoidBase
{
public:
	DECLARE_CLASS(CNPC_ArmedHumanoidBase, CNPC_HumanoidBase);

	void StartTouch(CBaseEntity *pOther) override;
	void HeisterTouch(CBaseEntity *pOther);

	void Spawn() override;
};

#endif
