//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FIRE_H
#define FIRE_H
#pragma once

#include "entityoutput.h"
#include "fire_smoke.h"
#include "plasma.h"

//Spawnflags
enum SFFire_t : unsigned short
{
	SF_FIRE_INFINITE =			0x00000001,
	SF_FIRE_SMOKELESS =			0x00000002,
	SF_FIRE_START_ON =			0x00000004,
	SF_FIRE_START_FULL =			0x00000008,
	SF_FIRE_DONT_DROP =			0x00000010,
	SF_FIRE_NO_GLOW =				0x00000020,
	SF_FIRE_DIE_PERMANENT =		0x00000080,
	SF_FIRE_VISIBLE_FROM_ABOVE =	0x00000100,
};

FLAGENUM_OPERATORS( SFFire_t, unsigned short )

//==================================================
// CFire
//==================================================

enum fireType_e : unsigned char
{
	FIRE_NATURAL = 0,
	FIRE_PLASMA,
};

//==================================================
// FireSystem
//==================================================
bool FireSystem_StartFire( const Vector &position, float fireHeight, float attack, float fuel, SFFire_t flags, CBaseEntity *owner, fireType_e type = FIRE_NATURAL);
void FireSystem_ExtinguishInRadius( const Vector &origin, float radius, float rate );
void FireSystem_AddHeatInRadius( const Vector &origin, float radius, float heat );

bool FireSystem_GetFireDamageDimensions( CBaseEntity *pFire, Vector *pFireMins, Vector *pFireMaxs );

#endif // FIRE_H
