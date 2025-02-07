//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TRIGGERS_SHARED_H
#define TRIGGERS_SHARED_H
#pragma once

#include "tier0/platform.h"

//
// Spawnflags
//
enum SFTrigger_t : uint64
{
	SF_TRIGGER_ALLOW_CLIENTS				= (1 << 0),		// Players can fire this trigger
	SF_TRIGGER_ALLOW_NPCS					= (1 << 1),		// NPCS can fire this trigger
	SF_TRIGGER_ALLOW_PUSHABLES				= (1 << 2),		// Pushables can fire this trigger
	SF_TRIGGER_ALLOW_PHYSICS				= (1 << 3),		// Physics objects can fire this trigger
	SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS		= (1 << 4),		// *if* NPCs can fire this trigger, this flag means only player allies do so
	SF_TRIGGER_ONLY_CLIENTS_IN_VEHICLES		= (1 << 5),		// *if* Players can fire this trigger, this flag means only players inside vehicles can 
	SF_TRIGGER_ALLOW_ALL					= (1 << 6),		// Everything can fire this trigger EXCEPT DEBRIS!
	SF_TRIGGER_ONLY_CLIENTS_OUT_OF_VEHICLES	= (1 << 7),	// *if* Players can fire this trigger, this flag means only players outside vehicles can 
	SF_TRIGGER_TOUCH_DEBRIS 					= (1 << 8),	// Will touch physics debris objects
	SF_TRIGGER_ONLY_NPCS_IN_VEHICLES		= (1 << 9),	// *if* NPCs can fire this trigger, only NPCs in vehicles do so (respects player ally flag too)
	SF_TRIGGER_DISALLOW_BOTS                = (1 << 10),   // Bots are not allowed to fire this trigger
	// and multiple component physobjs (car, blob...)
	SF_TRIGGER_ALLOW_ITEMS					= (1 << 11),	// MOVETYPE_FLYGRAVITY (Weapons, items, flares, etc.) can fire this trigger

	SF_TRIGGER_LAST_FLAG = SF_TRIGGER_ALLOW_ITEMS,
};

FLAGENUM_OPERATORS( SFTrigger_t, uint64 )

enum SFTriggerPush_t : uint64
{
	SF_TRIGGER_PUSH_ONCE						= (SF_TRIGGER_LAST_FLAG << 1),		// trigger_push removes itself after firing once
	SF_TRIGGER_PUSH_AFFECT_PLAYER_ON_LADDER	= (SF_TRIGGER_LAST_FLAG << 2),	// if pushed object is player on a ladder, then this disengages them from the ladder (HL2only)
	SF_TRIGGER_PUSH_USE_MASS				= (SF_TRIGGER_LAST_FLAG << 3),	// Correctly account for an entity's mass (CTriggerPush::Touch used to assume 100Kg)
};

FLAGENUM_OPERATORS( SFTriggerPush_t, uint64 )

// Spawnflags for CTriggerPlayerMovement
enum SFPlayerMovementTrigger_t : uint64
{
	SF_TRIGGER_MOVE_AUTODISABLE				= (SF_TRIGGER_LAST_FLAG << 1),	// Disable auto movement
	SF_TRIGGER_AUTO_DUCK						= (SF_TRIGGER_LAST_FLAG << 2),	// Duck automatically
	SF_TRIGGER_AUTO_WALK						= (SF_TRIGGER_LAST_FLAG << 3),
	SF_TRIGGER_DISABLE_JUMP					= (SF_TRIGGER_LAST_FLAG << 4),
};

FLAGENUM_OPERATORS( SFPlayerMovementTrigger_t, uint64 )

#endif // TRIGGERS_SHARED_H
