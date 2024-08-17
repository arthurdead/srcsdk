//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#ifndef COMBATWEAPON_H
#define COMBATWEAPON_H
#pragma once

#include "entityoutput.h"
#include "basecombatweapon_shared.h"

//-----------------------------------------------------------------------------
// Bullet types
//-----------------------------------------------------------------------------

// -----------------------------------------
//	Sounds
// -----------------------------------------

struct animevent_t;

extern void	SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

#endif // COMBATWEAPON_H
