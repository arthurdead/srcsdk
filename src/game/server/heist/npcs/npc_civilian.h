#ifndef NPC_CIVILIAN_H
#define NPC_CIVILIAN_H

#pragma once

#include "npc_humanoidbase.h"
#include "heist_shareddefs.h"

class CNPC_Civilian : public CNPC_HumanoidBase
{
public:
	DECLARE_CLASS(CNPC_Civilian, CNPC_HumanoidBase);

	DEFINE_CUSTOM_AI;

	void Precache() override;
	void Spawn() override;

	Class_T Classify() override
	{ return CLASS_CIVILIAN; }
};

#endif
