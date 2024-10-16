#ifndef NPC_HUMANOIDBASE_H
#define NPC_HUMANOIDBASE_H

#pragma once

#include "ai_baseactor.h"

class CNPC_HumanoidBase : public CAI_BaseActor
{
public:
	DECLARE_CLASS(CNPC_HumanoidBase, CAI_BaseActor);
	DECLARE_SERVERCLASS();

	CNPC_HumanoidBase();
	~CNPC_HumanoidBase() override;

	void Spawn() override;

private:
	void SuspicionThink();

protected:
	
};

#endif
