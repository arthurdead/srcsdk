//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NPCSTATE_H
#define AI_NPCSTATE_H

#pragma once

#include "tier0/platform.h"

enum NPC_STATE : unsigned char
{
	NPC_STATE_INVALID = (unsigned char)-1,

	NPC_STATE_NONE = 0,

	NPC_STATE_IDLE,
	NPC_STATE_ALERT,
	NPC_STATE_COMBAT,

	NPC_STATE_SCRIPT,
	NPC_STATE_PLAYDEAD,
	NPC_STATE_PRONE,				// When in clutches of barnacle
	NPC_STATE_DEAD
};

#endif // AI_NPCSTATE_H
