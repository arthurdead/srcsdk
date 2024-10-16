#ifndef NPC_CIVILIAN_H
#define NPC_CIVILIAN_H

#pragma once

#include "npc_humanoidbase.h"
#include "heist_shareddefs.h"

class CNPC_Cop : public CNPC_HumanoidBase
{
public:
	DECLARE_CLASS(CNPC_Cop, CNPC_HumanoidBase);

	void StartTouch(CBaseEntity *pOther) override;
	void HeisterTouch(CBaseEntity *pOther);

	void Precache() override;
	void Spawn() override;

	Class_T Classify() override
	{ return CLASS_POLICE; }
};

#endif
