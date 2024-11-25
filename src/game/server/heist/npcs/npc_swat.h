#ifndef NPC_LAWENFORCEMENT_H
#define NPC_LAWENFORCEMENT_H

#pragma once

#include "npc_armedhumanoidbase.h"
#include "heist_shareddefs.h"

class CNPC_Swat : public CNPC_ArmedHumanoidBase
{
public:
	DECLARE_CLASS(CNPC_Swat, CNPC_ArmedHumanoidBase);
	DECLARE_MAPENTITY();

	void Precache() override;
	void Spawn() override;

	Class_T Classify() override;

private:
	bool m_bHeavy;
};

#endif
