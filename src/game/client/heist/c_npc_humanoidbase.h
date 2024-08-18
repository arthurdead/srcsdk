#ifndef C_NPC_HUMANOIDBASE_H
#define C_NPC_HUMANOIDBASE_H

#pragma once

#include "c_ai_basenpc.h"

class C_NPC_HumanoidBase : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_HumanoidBase, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	C_NPC_HumanoidBase();

	int DrawModel(int flags) override;

protected:
	float m_flSuspicion[MAX_PLAYERS];
};

#endif
