#ifndef NPC_CIVILIAN_H
#define NPC_CIVILIAN_H

#pragma once

#include "npc_humanoidbase.h"

class CNPC_Cop : public CNPC_HumanoidBase
{
public:
	DECLARE_CLASS(CNPC_Cop, CNPC_HumanoidBase);

	DEFINE_CUSTOM_AI;

	void StartTouch(CBaseEntity *pOther) override;

	void Precache() override;
	void Spawn() override;

	Class_T Classify() override
	{ return CLASS_POLICE; }
};

#endif