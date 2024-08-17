#ifndef NPC_CIVILIAN_H
#define NPC_CIVILIAN_H

#pragma once

#include "ai_baseactor.h"

class CNPC_Civilian : public CAI_BaseActor
{
public:
	DECLARE_CLASS(CNPC_Civilian, CAI_BaseActor);

	DEFINE_CUSTOM_AI;

	void Precache() override;
	void Spawn() override;

	Class_T Classify() override
	{ return CLASS_CIVILIAN; }
};

#endif
