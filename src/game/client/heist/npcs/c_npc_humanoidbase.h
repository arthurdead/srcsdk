#ifndef C_NPC_HUMANOIDBASE_H
#define C_NPC_HUMANOIDBASE_H

#pragma once

#include "c_ai_basenpc.h"
#include "suspicioner.h"

class C_NPC_HumanoidBase : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_HumanoidBase, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	C_NPC_HumanoidBase();

	void Spawn() override;

protected:
	C_Suspicioner m_Suspicioner;
};

#endif
